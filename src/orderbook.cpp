#include "orderbook.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <algorithm>

void OrderBook::addOrder(OrderPtr order) {
    // No lock here - caller must hold lock
    active_orders[order->getOrderId()] = order;
    
    if (order->getSide() == OrderSide::BUY) {
        bids[order->getPrice()].push(order);
    } else {
        asks[order->getPrice()].push(order);
    }
}

std::vector<Trade> OrderBook::matchOrder(OrderPtr incoming_order) {
    std::unique_lock<std::shared_mutex> lock(book_mutex);
    
    std::vector<Trade> trades;
    active_orders[incoming_order->getOrderId()] = incoming_order;
    
    if (incoming_order->getSide() == OrderSide::BUY) {
        while (!incoming_order->isFilled() && !asks.empty()) {
            auto& [best_ask_price, ask_queue] = *asks.begin();
            
            if (incoming_order->getType() == OrderType::LIMIT_BUY && 
                best_ask_price > incoming_order->getPrice()) {
                break;
            }
            
            while (!ask_queue.empty() && !incoming_order->isFilled()) {
                OrderPtr match_order = ask_queue.front();
                
                uint64_t trade_qty = std::min(
                    incoming_order->getRemainingQuantity(),
                    match_order->getRemainingQuantity()
                );
                
                incoming_order->fill(trade_qty);
                match_order->fill(trade_qty);
                
                Trade trade;
                trade.buy_order_id = incoming_order->getOrderId();
                // Handle iceberg order replenishment
                if (match_order->isIceberg() && match_order->getVisibleQuantity() == 0 && !match_order->isFilled()) {
                    match_order->replenishDisplay();
                }
                trade.sell_order_id = match_order->getOrderId();
                trade.price = best_ask_price;
                trade.quantity = trade_qty;
                trade.timestamp = incoming_order->getTimestamp();
                trades.push_back(trade);
                trade_history.push_back(trade);
                
                if (match_order->isFilled()) {
                    active_orders.erase(match_order->getOrderId());
                    ask_queue.pop();
                }
            }
            
            if (ask_queue.empty()) {
                asks.erase(asks.begin());
            }
        }
    } else {
        while (!incoming_order->isFilled() && !bids.empty()) {
            auto& [best_bid_price, bid_queue] = *bids.begin();
            
            if (incoming_order->getType() == OrderType::LIMIT_SELL && 
                best_bid_price < incoming_order->getPrice()) {
                break;
            }
            
            while (!bid_queue.empty() && !incoming_order->isFilled()) {
                OrderPtr match_order = bid_queue.front();
                
                uint64_t trade_qty = std::min(
                    incoming_order->getRemainingQuantity(),
                    match_order->getRemainingQuantity()
                );
                
                incoming_order->fill(trade_qty);
                match_order->fill(trade_qty);
                
                Trade trade;
                trade.buy_order_id = match_order->getOrderId();
                trade.sell_order_id = incoming_order->getOrderId();
                // Handle iceberg order replenishment
                if (match_order->isIceberg() && match_order->getVisibleQuantity() == 0 && !match_order->isFilled()) {
                    match_order->replenishDisplay();
                }
                trade.price = best_bid_price;
                trade.quantity = trade_qty;
                trade.timestamp = incoming_order->getTimestamp();
                trades.push_back(trade);
                trade_history.push_back(trade);
                
                if (match_order->isFilled()) {
                    active_orders.erase(match_order->getOrderId());
                    bid_queue.pop();
                }
            }
            
            if (bid_queue.empty()) {
                bids.erase(bids.begin());
            }
        }
    }
    
    if (!incoming_order->isFilled() && 
        (incoming_order->getType() == OrderType::LIMIT_BUY || 
         incoming_order->getType() == OrderType::LIMIT_SELL)) {
        addOrder(incoming_order);
    } else if (incoming_order->isFilled()) {
        active_orders.erase(incoming_order->getOrderId());
    }
    
    return trades;
}

bool OrderBook::cancelOrder(uint64_t order_id) {
    std::unique_lock<std::shared_mutex> lock(book_mutex);
    
    auto it = active_orders.find(order_id);
    if (it == active_orders.end()) {
        return false;
    }
    
    OrderPtr order = it->second;
    double price = order->getPrice();
    OrderSide side = order->getSide();
    
    auto removeFromQueue = [&](std::queue<OrderPtr>& q) -> bool {
        std::queue<OrderPtr> temp_queue;
        bool found = false;
        
        while (!q.empty()) {
            OrderPtr front_order = q.front();
            q.pop();
            
            if (front_order->getOrderId() == order_id) {
                found = true;
            } else {
                temp_queue.push(front_order);
            }
        }
        
        while (!temp_queue.empty()) {
            q.push(temp_queue.front());
            temp_queue.pop();
        }
        
        return found;
    };
    
    bool removed = false;
    if (side == OrderSide::BUY) {
        auto bid_it = bids.find(price);
        if (bid_it != bids.end()) {
            removed = removeFromQueue(bid_it->second);
            if (bid_it->second.empty()) {
                bids.erase(bid_it);
            }
        }
    } else {
        auto ask_it = asks.find(price);
        if (ask_it != asks.end()) {
            removed = removeFromQueue(ask_it->second);
            if (ask_it->second.empty()) {
                asks.erase(ask_it);
            }
        }
    }
    
    if (removed) {
        active_orders.erase(it);
        return true;
    }
    
    return false;
}

bool OrderBook::modifyOrder(uint64_t order_id, double new_price, uint64_t new_quantity) {
    std::unique_lock<std::shared_mutex> lock(book_mutex);
    
    auto it = active_orders.find(order_id);
    if (it == active_orders.end()) {
        return false;
    }
    
    OrderPtr old_order = it->second;
    
    // Temporarily unlock to call cancelOrder which will reacquire
    lock.unlock();
    if (!cancelOrder(order_id)) {
        return false;
    }
    lock.lock();
    
    OrderPtr new_order = std::make_shared<Order>(
        old_order->getOrderId(),
        old_order->getTimestamp(),
        new_price,
        new_quantity,
        old_order->getType(),
        old_order->getSide()
    );
    
    addOrder(new_order);
    return true;
}

double OrderBook::getBestBid() const {
    std::shared_lock<std::shared_mutex> lock(book_mutex);
    if (bids.empty()) return 0.0;
    return bids.begin()->first;
}

double OrderBook::getBestAsk() const {
    std::shared_lock<std::shared_mutex> lock(book_mutex);
    if (asks.empty()) return std::numeric_limits<double>::max();
    return asks.begin()->first;
}

double OrderBook::getSpread() const {
    std::shared_lock<std::shared_mutex> lock(book_mutex);
    if (bids.empty() || asks.empty()) return 0.0;
    return getBestAsk() - getBestBid();
}

void OrderBook::printBook() const {
    std::shared_lock<std::shared_mutex> lock(book_mutex);
    
    std::cout << "\n========== ORDER BOOK ==========" << std::endl;
    std::cout << "ASKS (Sell Orders):" << std::endl;
    
    int count = 0;
    for (auto it = asks.rbegin(); it != asks.rend() && count < 5; ++it) {
        const auto& [price, queue] = *it;
        size_t total_qty = 0;
        std::queue<OrderPtr> temp_queue = queue;
        while (!temp_queue.empty()) {
            total_qty += temp_queue.front()->getRemainingQuantity();
            temp_queue.pop();
        }
        std::cout << "  " << std::fixed << std::setprecision(2) << price 
                  << " | " << total_qty << std::endl;
        count++;
    }
    
    std::cout << "--------------------------------" << std::endl;
    std::cout << "Spread: " << std::fixed << std::setprecision(2) 
              << getSpread() << std::endl;
    std::cout << "Active Orders: " << active_orders.size() << std::endl;
    std::cout << "--------------------------------" << std::endl;
    
    std::cout << "BIDS (Buy Orders):" << std::endl;
    count = 0;
    for (const auto& [price, queue] : bids) {
        if (count >= 5) break;
        size_t total_qty = 0;
        std::queue<OrderPtr> temp_queue = queue;
        while (!temp_queue.empty()) {
            total_qty += temp_queue.front()->getRemainingQuantity();
            temp_queue.pop();
        }
        std::cout << "  " << std::fixed << std::setprecision(2) << price 
                  << " | " << total_qty << std::endl;
        count++;
    }
    std::cout << "================================\n" << std::endl;
}

void OrderBook::printTrades() const {
    std::shared_lock<std::shared_mutex> lock(book_mutex);
    
    std::cout << "\n========== TRADE HISTORY ==========" << std::endl;
    for (const auto& trade : trade_history) {
        std::cout << "Trade: Buy#" << trade.buy_order_id 
                  << " <-> Sell#" << trade.sell_order_id
                  << " | " << trade.quantity << "@" 
                  << std::fixed << std::setprecision(2) << trade.price 
                  << std::endl;
    }
    std::cout << "===================================\n" << std::endl;
}

size_t OrderBook::getBidDepth() const {
    std::shared_lock<std::shared_mutex> lock(book_mutex);
    return bids.size();
}

size_t OrderBook::getAskDepth() const {
    std::shared_lock<std::shared_mutex> lock(book_mutex);
    return asks.size();
}

size_t OrderBook::getActiveOrderCount() const {
    std::shared_lock<std::shared_mutex> lock(book_mutex);
    return active_orders.size();
}

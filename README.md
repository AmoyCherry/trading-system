# trading-system

## What is trading?

There are some instruments and assets, like stocks, futures and cryptoconcurrencis.
People buy and sell these items from each other via the exchange. All bid orders and ask orders are sent to the exchange process.

- The Best Bid is the highest price that buyers are willing to buy.
- The Best Ask is the lowest price that sellers are willing to sell.
When `Best Bid >= Best Ask`, the orders can be matched.

The exchange handles those orders from both sides in the same queue sequentially. It processes one order at a time. If `Best Bid >= Best Ask`, it will match the orders exhaustedly. All unmatched orders remain in the exchange to be maintained, called as Limit Order Book. For each item in the LOB, there is  `Best Bid < Best Ask`.

The exchange broadcasts order updates to subscribers, including those quant shops. They receive updates and reconstruct the LOB on their local. Then they run their algos to send orders to the exchange.

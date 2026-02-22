# Minimal Matching Spec (v0)

## Order Types
- NewOrder(id, side, price, qty, symbol)
- Cancel(id)

## Validations
- qty must be > 0
- price must be > 0 (v0 only limit orders)
- NewOrder id must be unique among live orders; otherwise Reject(DuplicateOrderId)
- Cancel id must exist among live orders; otherwise Reject(UnknownOrderId)

## Matching Rules
- Price-time priority.
- Buy matches against best ask while best ask price <= buy price.
- Sell matches against best bid while best bid price >= sell price.
- Partial fills allowed.
- For each trade, emit two Fill events: one for taker (incoming), one for maker (resting).
- If incoming order has remaining qty after matching, it becomes resting at its limit price.

## Cancel Rules
- Cancel removes the live order if present and emits CancelAck(id).

## Invariants (must always hold after processing a message)
- No live order has qty <= 0.
- If both sides non-empty, best_bid < best_ask (book not crossed).
- FIFO at same price level.
- Deterministic replay: same input stream -> same output stream.

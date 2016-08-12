test mpsc ring buffer
===

This is a mpsc version of Dmitry Vyukov's [bounded mpmc queue](http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue)

As it turns out adding msgs to the ring buffer is NOT determinstic and
it may loop if
racing with other threads. But I believe if called with interrupts
disabled it is deterministic because the ISR should not be racing with
any other thread.

Of course the queue is bounded so ISR's must never all add only
add_non_blocking.

Remvoing elements from the queue is straight line code so thats
nice.

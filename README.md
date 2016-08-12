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

Take the following performance information with a huge grain of salt
but as would be expected the ring buffer is significantly faster for
the case of just adding/removing messages without contention.  It
takes about 2ns for the ring buffer, about 3ns for the instrusive
fifo and 4ns for the non-instrusive fifo:

Ring buffer:
``
$ nk@wink-desktop:~/prgs/test-mpscringbuff (master)
time ./test 16 1 0x100000
test client_count=16 loops=1 msg_count=1048576
     1 7fdd342f7700  multi_thread_msg:+client_count=16 loops=1 msg_count=1048576
     2 7fdd342f7700  multi_thread_msg: cmds_processed=528 msgs_processed=17827120 no_msgs=0 rbuf_full=0 mt_msgs_sent=16  mt_no_msgs=0
     3 7fdd342f7700  startup=0.572996
     4 7fdd342f7700  looping=0.000037
     5 7fdd342f7700  disconneting=0.000199
     6 7fdd342f7700  stopping=0.016524
     7 7fdd342f7700  complete=0.017415
     8 7fdd342f7700  processing=0.034s
     9 7fdd342f7700  cmds_per_sec=15450
    10 7fdd342f7700  ns_per_cmd=64724.5ns
    11 7fdd342f7700  msgs_per_sec=521649600
    12 7fdd342f7700  ns_per_msg=1.9ns
    13 7fdd342f7700  total=0.607
    14 7fdd342f7700  multi_thread_msg:-error=0

Success

real    0m0.609s
user    0m0.577s
sys     0m0.200s
```

Instrusive fifo:
```
wink@wink-desktop:~/prgs/test-mpscfifo-intrusive (master)
$ time ./test 16 1 0x100000
test client_count=16 loops=1 msg_count=1048576
     1 7f89d0fb2700  multi_thread_msg:+client_count=16 loops=1 msg_count=1048576
     2 7f89d0fb2700  multi_thread_msg: cmds_processed=528 msgs_processed=17827120 mt_msgs_sent=16 mt_no_msgs=0
     3 7f89d0fb2700  startup=0.521886
     4 7f89d0fb2700  looping=0.000088
     5 7f89d0fb2700  disconneting=0.000216
     6 7f89d0fb2700  stopping=0.022341
     7 7f89d0fb2700  complete=0.030247
     8 7f89d0fb2700  processing=0.053s
     9 7f89d0fb2700  cmds_per_sec=9982
    10 7f89d0fb2700  ns_per_cmd=100173.2ns
    11 7f89d0fb2700  msgs_per_sec=337051124
    12 7f89d0fb2700  ns_per_msg=3.0ns
    13 7f89d0fb2700  total=0.575
    14 7f89d0fb2700  multi_thread_msg:-error=0

Success

real    0m0.577s
user    0m0.690s
sys     0m0.163s
```

Non-instrusive fifo:
```
wink@wink-desktop:~/prgs/test-mpscfifo (master)
$ time ./test 16 1 0x100000
test client_count=16 loops=1 msg_count=1048576
     1 7f3ac97db700  multi_thread_msg:+client_count=16 loops=1 msg_count=1048576
     2 7f3ac97db700  multi_thread_msg: cmds_processed=528 msgs_processed=17827153 mt_msgs_sent=16 mt_no_msgs=0
     3 7f3ac97db700  startup=0.569066
     4 7f3ac97db700  looping=0.000079
     5 7f3ac97db700  disconneting=0.000171
     6 7f3ac97db700  stopping=0.037484
     7 7f3ac97db700  complete=0.034897
     8 7f3ac97db700  processing=0.073s
     9 7f3ac97db700  cmds_per_sec=7269
    10 7f3ac97db700  ns_per_cmd=137557.5ns
    11 7f3ac97db700  msgs_per_sec=245450492
    12 7f3ac97db700  ns_per_msg=4.1ns
    13 7f3ac97db700  total=0.642
    14 7f3ac97db700  multi_thread_msg:-error=0

Success

real	0m0.643s
user	0m0.910s
sys	0m0.200s
```

But things are somewhat different when we stress below the ns_per_msg
is slightly faster at 15.4ns as compared to 17.0ns and 17.5ns but looking
and the ns_per_cmd the ring buffer was at 65.6ns where as the fifo's
are at 34ns and 35ns so twice as fast. I need to do more instrumentation
to see exactly where the time is being spent.

Ring buffer:
```
wink@wink-desktop:~/prgs/test-mpscringbuff (master)
$ time ./test 12 10000000 0x10000
test client_count=12 loops=10000000 msg_count=65536
     1 7f509a5ab700  multi_thread_msg:+client_count=12 loops=10000000 msg_count=65536
     2 7f509a5ab700  multi_thread_msg: cmds_processed=344795626 msgs_processed=1468324282 no_msgs=118920 rbuf_full=770783028 mt_msgs_sent=93071341  mt_no_msgs=26928659
     3 7f509a5ab700  startup=0.061540
     4 7f509a5ab700  looping=22.538948
     5 7f509a5ab700  disconneting=0.074059
     6 7f509a5ab700  stopping=0.002306
     7 7f509a5ab700  complete=0.001568
     8 7f509a5ab700  processing=22.617s
     9 7f509a5ab700  cmds_per_sec=15245056
    10 7f509a5ab700  ns_per_cmd=65.6ns
    11 7f509a5ab700  msgs_per_sec=64921606
    12 7f509a5ab700  ns_per_msg=15.4ns
    13 7f509a5ab700  total=22.678
    14 7f509a5ab700  multi_thread_msg:-error=0

Success

real    0m22.680s
user    4m11.557s
sys     0m11.773s
```

Instrustive fifo:
```
wink@wink-desktop:~/prgs/test-mpscfifo-intrusive (master)
$ time ./test 12 10000000 0x10000
test client_count=12 loops=10000000 msg_count=65536
     1 7fa157b44700  multi_thread_msg:+client_count=12 loops=10000000 msg_count=65536
     2 7fa157b44700  multi_thread_msg: cmds_processed=908602454 msgs_processed=1818057032 mt_msgs_sent=79213311 mt_no_msgs=40786689
     3 7fa157b44700  startup=0.043647
     4 7fa157b44700  looping=30.388611
     5 7fa157b44700  disconneting=0.461896
     6 7fa157b44700  stopping=0.006666
     7 7fa157b44700  complete=0.004913
     8 7fa157b44700  processing=30.862s
     9 7fa157b44700  cmds_per_sec=29440732
    10 7fa157b44700  ns_per_cmd=34.0ns
    11 7fa157b44700  msgs_per_sec=58909076
    12 7fa157b44700  ns_per_msg=17.0ns
    13 7fa157b44700  total=30.906
    14 7fa157b44700  multi_thread_msg:-error=0

Success

real    0m30.907s
user    5m17.633s
sys     0m38.220s
```

Non-instrusive fifo:
```
$ time ./test 12 10000000 0x10000
test client_count=12 loops=10000000 msg_count=65536
     1 7f1fb6c67700  multi_thread_msg:+client_count=12 loops=10000000 msg_count=65536
     2 7f1fb6c67700  multi_thread_msg: cmds_processed=801567192 msgs_processed=1603986533 mt_msgs_sent=70089550 mt_no_msgs=49910450
     3 7f1fb6c67700  startup=0.043761
     4 7f1fb6c67700  looping=27.879957
     5 7f1fb6c67700  disconneting=0.184048
     6 7f1fb6c67700  stopping=0.008817
     7 7f1fb6c67700  complete=0.006561
     8 7f1fb6c67700  processing=28.079s
     9 7f1fb6c67700  cmds_per_sec=28546468
    10 7f1fb6c67700  ns_per_cmd=35.0ns
    11 7f1fb6c67700  msgs_per_sec=57123285
    12 7f1fb6c67700  ns_per_msg=17.5ns
    13 7f1fb6c67700  total=28.123
    14 7f1fb6c67700  multi_thread_msg:-error=0

Success

real    0m28.125s
user    4m45.520s
sys     0m38.743s
```

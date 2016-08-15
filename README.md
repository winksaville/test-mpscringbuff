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
especially as the ranges run to run are not consistent enough for
my liking as I'm seeing 10% range difference between some runs.

Anyway, my previous data here was only taking into account the time to
remove the items from the lists. Here we're timing both add and remove
and for some reason the ring buffer is not fastest. More investigation
is needed, most likely I've made a mistake!!! But the ns_per_msg is:


18.1ns for non-instrusive
17.1ns for instrusive
17.6ns for ring buffer

```
$ make ; time ./test 12 10000000 0x100000
make: Nothing to be done for 'all'.
test client_count=12 loops=10000000 msg_count=1048576
     1 7f579536f700  multi_thread_msg:+client_count=12 loops=10000000 msg_count=1048576
     2 7f579536f700  multi_thread_msg: cmds_processed=981576981 msgs_processed=1976785631 mt_msgs_sent=84154773 mt_no_msgs=35845227
     3 7f579536f700  startup=0.472809
     4 7f579536f700  looping=29.196087
     5 7f579536f700  disconneting=6.358297
     6 7f579536f700  stopping=0.013630
     7 7f579536f700  complete=0.188888
     8 7f579536f700  processing=35.757s
     9 7f579536f700  cmds_per_sec=27451398.811
    10 7f579536f700  ns_per_cmd=36.4ns
    11 7f579536f700  msgs_per_sec=55284029.445
    12 7f579536f700  ns_per_msg=18.1ns
    13 7f579536f700  total=36.230
    14 7f579536f700  multi_thread_msg:-error=0

Success

real	0m36.231s
user	6m2.397s
sys	0m36.550s
wink@wink-desktop:~/prgs/test-mpscfifo (master)
$ cd ../test-mpscfifo-intrusive/
wink@wink-desktop:~/prgs/test-mpscfifo-intrusive (master)
$ make ; time ./test 12 10000000 0x100000
make: Nothing to be done for 'all'.
test client_count=12 loops=10000000 msg_count=1048576
     1 7f934b9ce700  multi_thread_msg:+client_count=12 loops=10000000 msg_count=1048576
     2 7f934b9ce700  multi_thread_msg: cmds_processed=1113542953 msgs_processed=2240717550 mt_msgs_sent=94700012 mt_no_msgs=25299988
     3 7f934b9ce700  startup=0.367424
     4 7f934b9ce700  looping=31.373848
     5 7f934b9ce700  disconneting=6.688196
     6 7f934b9ce700  stopping=0.014195
     7 7f934b9ce700  complete=0.167063
     8 7f934b9ce700  processing=38.243s
     9 7f934b9ce700  cmds_per_sec=29117332.529
    10 7f934b9ce700  ns_per_cmd=34.3ns
    11 7f934b9ce700  msgs_per_sec=58591110.322
    12 7f934b9ce700  ns_per_msg=17.1ns
    13 7f934b9ce700  total=38.611
    14 7f934b9ce700  multi_thread_msg:-error=0

Success

real	0m38.612s
user	6m34.120s
sys	0m32.617s
wink@wink-desktop:~/prgs/test-mpscfifo-intrusive (master)
$ cd ../test-mpscringbuff/
wink@wink-desktop:~/prgs/test-mpscringbuff (master)
$ make ; time ./test 12 10000000 0x100000
make: Nothing to be done for 'all'.
test client_count=12 loops=10000000 msg_count=1048576
     1 7fd786154700  multi_thread_msg:+client_count=12 loops=10000000 msg_count=1048576
     2 7fd786154700  multi_thread_msg: cmds_processed=565185445 msgs_processed=1577541210 no_msgs=21455 rbuf_full=428878827 mt_msgs_sent=82854646  mt_no_msgs=37145354
     3 7fd786154700  startup=0.435703
     4 7fd786154700  looping=27.023111
     5 7fd786154700  disconneting=0.702559
     6 7fd786154700  stopping=0.006119
     7 7fd786154700  complete=0.011889
     8 7fd786154700  processing=27.744s
     9 7fd786154700  cmds_per_sec=20371683.984
    10 7fd786154700  ns_per_cmd=49.1ns
    11 7fd786154700  msgs_per_sec=56861285.594
    12 7fd786154700  ns_per_msg=17.6ns
    13 7fd786154700  total=28.179
    14 7fd786154700  multi_thread_msg:-error=0

Success

real	0m28.181s
user	5m12.007s
sys	0m12.020s
```

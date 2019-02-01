# tinymq
VxWorks-like lightweight message queue for Windows

The tinymq implements the functions about a message queue which is similar with
the Wind River VxWorks kernel message queue.

The memory layout for tinymq message queue:
<pre><code>
----------------   -----------------------------------------------------------
| local memory |-->|                     shared memory                       |
----------------   -----------------------------------------------------------
       ^                                     ^
       |                                     |
----------------   -----------------------------------------------------------
|    MSG_Q     |   | MSG_SM | MSG_NODE list |       message queue data       |
----------------   -----------------------------------------------------------
                                    ^                         ^
                                    |                         |
             ---------------------------------------------    |
             | MSG_NODE1 | MSG_NODE2 | ... | MSG_NODE(N) |    |
             ---------------------------------------------    |
                                                              |
                                              ---------------------------------
                                              | data1 | data2 | ... | data(N) |
                                              ---------------------------------
</code></pre>

The message queue is divided into two parts in memory, they are local memory and
shared memory, and these two parts are not closed by. The local memory can be
accessed in an process, also can be accessed between threads in a process.
The shared memory can be accessed between threads in a process if the message
queue name is NULL; or can be accessed between processes if the message queue
name is not NULL. 

For each tinymq, the structure MSG_Q which desribe the header of
the queue is stored in local memory; and the structure MSG_SM, MSG_NODE list and
message queue data are stored in shared memory.

The structure MSG_Q saves the kernel objects handlers and the shared memory
address; MSG_SM saves the message queue attributes; MSG_NODE list saves all the
nodes for the message queue, and each node saves the attributes of a message;
the message queue data area saves the data for all the messages.

1. get_nnlist
2. erasure code read holl file into memory, aweful implemention
先切片到64M
64M进行纠删码得到7+5的纠删码分块
3. braft get node status is more gracefull

Callbacks
1. 只有namenode master能够执行propose

1. nn master change：通知datanode
2. 

braft会在超过半数的on_apply()成功后投票通过，而不是先投票通过再执行

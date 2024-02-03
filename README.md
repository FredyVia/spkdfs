1. get_nnlist
2. sqlite
3. braft get node status is more gracefull

Callbacks
1. 只有namenode master能够执行propose

1. nn master change：通知datanode
2. 

braft会在超过半数的on_apply()成功后投票通过，而不是先投票通过再执行
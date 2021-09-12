#define	socket	bind        // 把socket.S中的socket替换掉
#define	NARGS	3
#define NO_WEAK_ALIAS	1
#include <socket.S>
weak_alias (bind, __bind)

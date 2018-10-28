
#ifndef	__BLUETOOTH__
#define	__BLUETOOTH__
#include	<stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
bool	bluetooth_connect (void);
int	bluetooth_read	  (char *, int);
int	bluetooth_write	  (char *, int);
void	bluetooth_terminate	(void);
#ifdef __cplusplus
}
#endif
#endif


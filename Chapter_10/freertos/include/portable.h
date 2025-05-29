#ifndef _PORTABLE_H
#define _PORTABLE_H

#include "portmacro.h"
#include "projdefs.h"

StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TASKFunction_t pxCode, void *pvParameters);
BaseType_t xPortStartScheduler( void );

#endif
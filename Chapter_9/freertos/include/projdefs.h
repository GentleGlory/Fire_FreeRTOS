#ifndef _PROJDEF_H
#define _PROJDEF_H


typedef void ( *TASKFunction_t ) ( void * );

#define pdFALSE    ( ( BaseType_t ) 0 )
#define pdTRUE     ( ( BaseType_t ) 1 )

#define pdPass    ( pdTRUE )   
#define pdFAIL    ( pdFALSE )   

#endif
#include "list.h"

List_t xList;

ListItem_t List_Item1;
ListItem_t List_Item2;
ListItem_t List_Item3;


int main(void){

    vListInitialise( &xList );
    
    vListInitialiseItem( &List_Item1 );
    List_Item1.xItemValue = 1;
    
    vListInitialiseItem( &List_Item2 );
    List_Item2.xItemValue = 2;

    vListInitialiseItem( &List_Item3 );
    List_Item3.xItemValue = 3;
    
    vListInsert( &xList, &List_Item3);
    vListInsert( &xList, &List_Item1);
    vListInsert( &xList, &List_Item2);
      
    for(;;){
		
		}	
    return 0;
}
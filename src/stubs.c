#include "game.h"
#include "cards.h"
s8 map_pending_encounter;
void map_screen(void){
    for(;;){
        txt_clear(); ui_clear();
        txt_put(2,2,"MAP STUB",CLR_GREEN);
        txt_put(2,4,"SELECT: DECK  B: TITLE",CLR_GRAY);
        for(;;){
            vsync(); key_poll();
            if(key_hit(KEY_SELECT)){ deck_browse("DECK",0); break; }
            if(key_hit(KEY_B)){ gstate=ST_TITLE; return; }
        }
    }
}
void combat_screen(int e){ (void)e; gstate=ST_MAP; }
void reward_screen(int e){ (void)e; gstate=ST_MAP; }
void rest_screen(void){ gstate=ST_MAP; }
void shop_screen(void){ gstate=ST_MAP; }
void event_screen(void){ gstate=ST_MAP; }
void treasure_screen(void){ gstate=ST_MAP; }
void gameover_screen(void){ gstate=ST_TITLE; }
void victory_screen(void){ gstate=ST_TITLE; }
int map_current_room(void){ return ROOM_MONSTER; }
void map_generate(void){}

#include "game.h"
#include "cards.h"
s8 map_pending_encounter;
void map_screen(void){
    static int enc = 0;
    for(;;){
        txt_clear(); ui_clear();
        txt_put(2,2,"MAP STUB",CLR_GREEN);
        txt_put(2,4,"A: FIGHT ENC",CLR_WHITE); txt_int(15,4,enc,CLR_YELLOW);
        txt_put(2,5,"L/R CHANGE ENC",CLR_GRAY);
        txt_put(2,6,"SELECT: DECK  B: TITLE",CLR_GRAY);
        for(;;){
            vsync(); key_poll();
            if(key_hit(KEY_L)&&enc>0){ enc--; sfx_blip(); break; }
            if(key_hit(KEY_R)&&enc<12){ enc++; sfx_blip(); break; }
            if(key_hit(KEY_A)){ map_pending_encounter=enc; gstate=ST_COMBAT; return; }
            if(key_hit(KEY_SELECT)){ deck_browse("DECK",0); break; }
            if(key_hit(KEY_B)){ gstate=ST_TITLE; return; }
        }
    }
}
void reward_screen(int e){ (void)e; gstate=ST_MAP; }
void rest_screen(void){ gstate=ST_MAP; }
void shop_screen(void){ gstate=ST_MAP; }
void event_screen(void){ gstate=ST_MAP; }
void treasure_screen(void){ gstate=ST_MAP; }
void gameover_screen(void){ gstate=ST_TITLE; }
void victory_screen(void){ gstate=ST_TITLE; }
int map_current_room(void){ return ROOM_MONSTER; }
void map_generate(void){}

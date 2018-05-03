#include <iostream>
#include <stdint.h>
#include <cassert>


#define TS  8
#define TILE_SIZE  8

using namespace std;

char pacman_bm[] =
 "00111100"
 "01111110"
 "11111111"
 "11000000"
 "11100000"
 "11111111"
 "01111110"
 "00111100";



int main(){
    char matrix[TS*TS];
    char matrix_out[TS*TS];
    for(int i = 0; i < sizeof(matrix); ++i){
        matrix[i] = i;
    } 

    for(int x = 0; x < TS; ++x){ 
        for(int y = 0; y < TS; ++y){
                matrix_out[y * TS + x] = matrix[(TS - 1 - x)*TS + y];//DOWN   
            //matrix_out[y * TS + x] = matrix[(TS - 1 - y)*TS + TS-x-1];//LEFT
            //matrix_out[y * TS + x] = matrix[x*TS + TS - 1 - y];//UP   
        }
    } 

    for(int y = 0; y < TS; ++y){ 
        for(int x = 0; x < TS; ++x){
            cout << hex << (int) matrix[y * TS + x] << " ";  
        }
        cout << endl;
    } 
    cout << endl;
    cout << endl;

    for(int y = 0; y < TS; ++y){ 
        for(int x = 0; x < TS; ++x){
            cout << hex << (int) matrix_out[y * TS + x] << " ";  
        }
        cout << endl;
    } 

    cout << endl;

    uint8_t bm[8];
    for(int i = 0; i < 8; ++i) bm[i] = 0;
    
    for(int x = 0; x < 8; ++x){ 
        for(int y = 0; y < 8; ++y){
                 if(pacman_bm[y * 8 + x] == '1'){
                     bm[x] |= (1 << y);
                    }
                cout << pacman_bm[y * 8 + x];
            }
        cout << endl;
    }
        cout << endl;
    
    for(int i = 0; i < 8; ++i) cout << " " << hex << (int) bm[i];
    cout << endl;
    
   uint8_t cont = 0x82;
    uint16_t px = 8*8;
    uint16_t py = 8;
    for(int h = 0; h < 8; ++h){
        py = h * 8;
        cout << endl;    
    
        for(int i = 0; i < 16; ++i){
            px = i * 8;
 //    cout << "POS: " << px << " " << py << endl;
     //uint16_t idx = ((py * 128)/8  + px)/64  ; // range = 128
     uint16_t idx = ((py * 128)/8  + px)  ; // range = 128
  //   cout << "index: " << dec <<  (uint16_t) idx/8 << endl;
  //   cout << " " << dec <<  (uint16_t) idx ;
     uint16_t bit = (uint16_t)((uint16_t)(idx/64)  * 64) ;
  //   cout << "bit: " << dec << (int) bit <<endl;
     bit = (uint16_t) idx - bit;   
        cout << bit/8 << " ";   
  //   cout << "bit: " << dec << (int) bit <<endl;
  //   cout << "content: " << (bool)(cont & (1 << bit)) << endl;
  //      assert(bit < 8);
  //      assert(idx/8 < 16);
        }
    }
        cout << endl;    


    uint16_t i = 15;
    uint16_t bit = 7;

    px = (uint16_t)(bit * 8 + i * 64);
    py = px / 128 * 8;
    px = px % 128 ;

    cout << dec << px << endl;
    cout << dec << py << endl;


}


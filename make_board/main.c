/*
 * Copyright 2002-2010 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */
//gcc -o out main.c -lpng

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#define PNG_DEBUG 3
#include <png.h>

void abort_(const char * s, ...){
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        abort();
}

int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;

void read_png_file(char* file_name)
{
        char header[8];    // 8 is the maximum size that can be checked

        /* open file and test for it being a png */
        FILE *fp = fopen(file_name, "rb");
        if (!fp)
                abort_("[read_png_file] File %s could not be opened for reading", file_name);
        fread(header, 1, 8, fp);
        if (png_sig_cmp(header, 0, 8))
                abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


        /* initialize stuff */
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[read_png_file] png_create_read_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[read_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during init_io");

        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);

        png_read_info(png_ptr, info_ptr);

        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        number_of_passes = png_set_interlace_handling(png_ptr);
        png_read_update_info(png_ptr, info_ptr);


        /* read file */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during read_image");

        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        for (y=0; y<height; y++)
                row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

        png_read_image(png_ptr, row_pointers);

        fclose(fp);
}


void write_png_file(char* file_name)
{
        /* create file */
        FILE *fp = fopen(file_name, "wb");
        if (!fp)
                abort_("[write_png_file] File %s could not be opened for writing", file_name);


        /* initialize stuff */
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[write_png_file] png_create_write_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[write_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during init_io");

        png_init_io(png_ptr, fp);


        /* write header */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during writing header");

        png_set_IHDR(png_ptr, info_ptr, width, height,
                     bit_depth, color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);


        /* write bytes */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during writing bytes");

        png_write_image(png_ptr, row_pointers);


        /* end write */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during end of write");

        png_write_end(png_ptr, NULL);

        /* cleanup heap allocation */
        for (y=0; y<height; y++)
                free(row_pointers[y]);
        free(row_pointers);

        fclose(fp);
}


void process_file(char* file_name, int tile_size){
        if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB)
                abort_("[process_file] input file is PNG_COLOR_TYPE_RGB but must be PNG_COLOR_TYPE_RGBA "
                       "(lacks the alpha channel)");

        if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGBA)
                abort_("[process_file] color_type of input file must be PNG_COLOR_TYPE_RGBA (%d) (is %d)",
                       PNG_COLOR_TYPE_RGBA, png_get_color_type(png_ptr, info_ptr));
        //int board_width = (width - offset_left - offset_right)/tile_size;
        //int board_height = (height - offset_top - offset_bottom)/tile_size;
        unsigned char board[width * height];
        //unsigned char pages[(int)(height/8) * width];
        for(int i = 0; i < sizeof(board); ++i) board[i] = 0;
        int pos_pacman, pos_ghost;

        for (y=0; y<height; y++) {
                png_byte* row = row_pointers[y];
                for (x=0; x<width; x++) {
                        png_byte* ptr = &(row[x*4]);
                        //unsigned int page = (int)(y/8) * width + x;     
                        //unsigned int bit = y%8;     
                        if(ptr[0] && !ptr[1] && !ptr[2])
                            pos_ghost = x + y * width;
                        else if(!ptr[0] && ptr[1] && !ptr[2])
                            pos_pacman = x + y *width;
                        else if(ptr[0] < 50 && ptr[1] < 50 && ptr[2] < 50){
                            printf("Pixel at position [ %d - %d ] has RGBA values: %d - %d - %d - %d\n",
                               x, y, ptr[0], ptr[1], ptr[2], ptr[3]);
                             /*   
                            printf("page: %d - bit: %d - byte: %d\n",page, bit, pages[page]);
                            pages[page] |= (1 << bit); 
                           */ 
                            board[x + y * width] = 1;
                        }
                        /* set red value to 0 and green value to the blue one */
                        //ptr[0] = 0;
                        //ptr[1] = ptr[2];
                }
        }
    FILE *f = fopen(file_name, "w");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    /* print some text */
    fprintf(f, "#ifndef CONCAT_H\n#define CONCAT_H\n\n");
    fprintf(f, "#include<stdint.h>\n");
    fprintf(f, "\n#define TILE_SIZE %d\n", tile_size);
    fprintf(f, "#define BOARD_HEIGHT %d\n", 64/tile_size);
    fprintf(f, "#define BOARD_WIDTH %d\n", 128/tile_size);
    fprintf(f, "#define POS_PACMAN %d\n", pos_pacman);
    fprintf(f, "#define POS_GHOST %d\n\n", pos_ghost);


    
/*
    fprintf(f, "static const __flash uint8_t board[] = {");
    
    for(int i = 0; i < sizeof(pages); ++i){
        if(i != sizeof(pages)-1) fprintf(f, "0x%.2x, ", pages[i]);
        else fprintf(f, "0x%.2x };", pages[i]);
        if(i && !( i % 10) ) fprintf(f,"\n"); 
    }

    fprintf(f, "\n#endif\n");
*/
    //BOARD
    fprintf(f, "static const __flash uint8_t board[] = {\n\t");
    int bits = 8; 
    for(int i = 0; i < sizeof(board); i+=bits){
        if(i && !( i % 16 )) fprintf(f,"\n\t"); 
        uint8_t line = 0;
        for(int bit = 0; bit < bits; ++bit){
            line |= (board[i+bit] << bit);
        }
        if(i < sizeof(board)-9) 
            fprintf(f, "0x%.2x, ", line);
        else 
            fprintf(f, "0x%.2x};", line);

    }
    //POINTS
    fprintf(f, "\n\nuint8_t dots[] = {\n\t");
    for(int i = 0; i < sizeof(board); i+=bits){
        if(i && !( i % 16 )) fprintf(f,"\n\t"); 
        uint8_t line = 0;
        for(int bit = 0; bit < bits; ++bit){
            int b = 0;    
            if(!board[i+bit]) b = 1;
                
            line |= (b << bit);
        }
        if(i < sizeof(board)-9) 
            fprintf(f, "0x%.2x, ", line);
        else 
            fprintf(f, "0x%.2x};", line);

    }

    fprintf(f, "\n#endif\n");




    fclose(f);

}


int main(int argc, char **argv)
{
        if (argc != 4)
                abort_("Usage: program_name <file_in> <file_out> <tile_size>");

        read_png_file(argv[1]);
    
        process_file(argv[2], atoi(argv[3]));
//        write_png_file(argv[2]);

        return 0;
}


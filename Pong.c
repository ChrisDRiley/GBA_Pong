
/* the width and height of the screen */
#define WIDTH 240
#define HEIGHT 160

/* these identifiers define different bit positions of the display control */
#define MODE4 0x0004
#define BG2 0x0400

/* this bit indicates whether to display the front or the back buffer
 * this allows us to refer to bit 4 of the display_control register */
#define SHOW_BACK 0x10;

/* the screen is simply a pointer into memory at a specific address this
 *  * pointer points to 16-bit colors of which there are 240x160 */
volatile unsigned short* screen = (volatile unsigned short*) 0x6000000;

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the address of the color palette used in graphics mode 4 */
volatile unsigned short* palette = (volatile unsigned short*) 0x5000000;

/* pointers to the front and back buffers - the front buffer is the start
 * of the screen array and the back buffer is a pointer to the second half */
volatile unsigned short* front_buffer = (volatile unsigned short*) 0x6000000;
volatile unsigned short* back_buffer = (volatile unsigned short*)  0x600A000;

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* the bit positions indicate each button - the first bit is for A, second for
 * B, and so on, each constant below can be ANDED into the register to get the
 * status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

/* the scanline counter is a memory cell which is updated to indicate how
 * much of the screen has been drawn */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank() {
    /* wait until all 160 lines have been updated */
    while (*scanline_counter < 160) { }
}

/* this function checks whether a particular button has been pressed */
unsigned char button_pressed(unsigned short button) {
    /* and the button register with the button constant we want */
    unsigned short pressed = *buttons & button;

    /* if this value is zero, then it's not pressed */
    if (pressed == 0) {
        return 1;
    } else {
        return 0;
    }
}

/* keep track of the next palette index */
int next_palette_index = 0;

/*
 * function which adds a color to the palette and returns the
 * index to it
 */
unsigned char add_color(unsigned char r, unsigned char g, unsigned char b) {
    unsigned short color = b << 10;
    color += g << 5;
    color += r;

    /* add the color to the palette */
    palette[next_palette_index] = color;

    /* increment the index */
    next_palette_index++;

    /* return index of color just added */
    return next_palette_index - 1;
}

/* a colored square */
struct square {
    unsigned short x, y, sizex, sizey,dx,dy;
    unsigned char color;
};

/* put a pixel on the screen in mode 4 */
void put_pixel(volatile unsigned short* buffer, int row, int col, unsigned char color) {
    /* find the offset which is the regular offset divided by two */
    unsigned short offset = (row * WIDTH + col) >> 1;

    /* read the existing pixel which is there */
    unsigned short pixel = buffer[offset];

    /* if it's an odd column */
    if (col & 1) {
        /* put it in the left half of the short */
        buffer[offset] = (color << 8) | (pixel & 0x00ff);
    } else {
        /* it's even, put it in the left half */
        buffer[offset] = (pixel & 0xff00) | color;
    }
}

/* draw a square onto the screen */
void draw_square(volatile unsigned short* buffer, struct square* s) {
    short row, col;
    /* for each row of the square */
    for (row = s->y; row < (s->y + s->sizey); row++) {
        /* loop through each column of the square */
        for (col = s->x; col < (s->x + s->sizex); col++) {
            /* set the screen location to this color */
            put_pixel(buffer, row, col, s->color);
        }
    }
}

/* clear the screen right around the square */
void update_screen(volatile unsigned short* buffer, unsigned short color, struct square* s) {
    short row, col;
    for (row = s->y - 3; row < (s->y + s->sizey + 3); row++) {
        for (col = s->x - 3; col < (s->x + s->sizex + 3); col++) {
            put_pixel(buffer, row, col, color);
        }
    }
}

/* this function takes a video buffer and returns to you the other one */
volatile unsigned short* flip_buffers(volatile unsigned short* buffer) {
    /* if the back buffer is up, return that */
    if(buffer == front_buffer) {
        /* clear back buffer bit and return back buffer pointer */
        *display_control &= ~SHOW_BACK;
        return back_buffer;
    } else {
        /* set back buffer bit and return front buffer */
        *display_control |= SHOW_BACK;
        return front_buffer;
    }
}

/* handle the buttons which are pressed down */
void handle_buttons(struct square* s) {
    /* move the square with the arrow keys */
    if (button_pressed(BUTTON_DOWN)) {
	if(s->y != HEIGHT-17){
		s->y += 1;
	}
    }
    if (button_pressed(BUTTON_UP)) {
	if(s->y != 1){
		s->y -= 1;
	}
    }
    if (button_pressed(BUTTON_RIGHT)) {
        s->x += 0;
    }
    if (button_pressed(BUTTON_LEFT)) {
        s->x -= 0;
    }
}

/* clear the screen to black */
void clear_screen(volatile unsigned short* buffer, unsigned short color) {
    unsigned short row, col;
    /* set each pixel black */
    for (row = 0; row < HEIGHT; row++) {
        for (col = 0; col < WIDTH; col++) {
            put_pixel(buffer, row, col, color);
        }
    }
}

void move_player_two(struct square* s, int* dir){

	if(s->y == 143 && *dir == 1){
		*dir = -1;
	}
	else if(s->y == 1 && *dir == -1){
		*dir = 1;
	}
	s->y += *dir;
}

void draw_score(int ps, int cs, volatile unsigned short* buffer, unsigned short color){
	if(ps != 0){
		for(int i = 1; i < ps; i++){
			for(int row = 5; row < 25; row++){
				for(int col = (10*i)+15; col < ((10*i) + 17); col++){
					put_pixel(buffer,row,col,color);
				}
			}
		}
	}
	if(cs != 0){
		for(int i = 1; i < cs; i++){
			for(int row = 5; row < 25; row++){
				for(int col = 195-(10*i); col < 197-(10*i); col++){
					put_pixel(buffer, row, col, color);
				}
			}
		}
	}
}

void clear_score(volatile unsigned short* buffer, unsigned short color){
		for(int i = 1; i < 6; i++){
			for(int row = 5; row < 25; row++){
				for(int col = (10*i)+15; col < ((10*i) + 17); col++){
					put_pixel(buffer,row,col,color);
				}
			}
		}
		for(int i = 1; i < 6; i++){
			for(int row = 5; row < 25; row++){
				for(int col = 195-(10*i); col < 197-(10*i); col++){
					put_pixel(buffer, row, col, color);
				}
			}
		}
}


void move_ball(struct square* b){
	b->x += b->dx;
	b->y += b->dy;
}
int check_bounds(struct square* b, struct square* s, struct square* s2, int* pscored, int* cscored){
	if(b->y == 0 || b->y == HEIGHT){
		b->dy *= -1;
	}
	if((b->x - 3 == s->x && (b->y >= s->y && b->y <= (s->y + 20))) || b->x + 3 == s2->x && (b->y >= s2->y && b->y <= (s2->y + 20))){
		b->dx *= -1;
	}
	if(b->x == WIDTH){
		b->x = 100;
		b->y = 50;
		b->dx = -1;
		b->dy = -1;
		(*pscored)+=1;
		return 1;
	}
	else if(b->x == 0){
		b->x = 100;
		b->y = 50;
		b->dx = 1;
		b->dy = 1;
		(*cscored)+=1;
		return 1;
	}
	return 0;
}

void clear_sides(volatile unsigned short* buffer, unsigned short color){
	for(int row = 0; row < HEIGHT; row++){
		for(int col = 0; col < 10; col++){
			put_pixel(buffer,row,col,color);
		}
	}
	for(int row = 0; row < HEIGHT; row++){
		for(int col = 210; col < WIDTH; col++){
			put_pixel(buffer,row,col,color);
		}
	}
}


/* the main function */
int main() {
    /* we set the mode to mode 4 with bg2 on */
    *display_control = MODE4 | BG2;

    /* make a green paddle */
    struct square s = {10, 10, 3,15,0,0, add_color(0, 20, 2)};
    /* make a red paddle for computer */
    struct square s2 = {200,10,3,15,0,0, add_color(20,0,0)}; 
    /* make a white ball */
    struct square b = {100, 5, 3,3,-1,-1, add_color(20,20,20)};
    
    /* Set initial direction of computer paddle */
    int direction = 1;
    int* ptr = &direction;
	
    /* Set initial player scores */
    int pscored = 1;
    int cscored = 1;
    int* psptr = &pscored;
    int* csptr = &cscored;
    int scored = 0;

    /* add black and blue to the palette */
    unsigned char black = add_color(0, 0, 0);
    unsigned char blue = add_color(2,0,20);
	
    /* the buffer we start with */
    volatile unsigned short* buffer = front_buffer;

    /* clear whole screen first */
    clear_screen(front_buffer, black);
    clear_screen(back_buffer, black);
	
    /* loop forever */
    while (1) {
        /* clear the screen - only the areas around the square! */
        update_screen(buffer, black, &s);
	update_screen(buffer, black, &s2);
	update_screen(buffer, black, &b);
	
	if(pscored == 6 || cscored == 6){
		pscored = 1;
		cscored = 1;
		clear_score(front_buffer,black);
		clear_score(back_buffer,black);
	}
        /* draw the square */
        draw_square(buffer, &s);
	draw_square(buffer, &s2);
	draw_square(buffer, &b);

        /* handle button input */
        handle_buttons(&s);
	
	/* Move the computer controlled paddle */
	move_player_two(&s2, ptr);
	
	move_ball(&b);
	scored = check_bounds(&b,&s,&s2, psptr, csptr);
	if(scored == 1){
		clear_sides(front_buffer,black);
		clear_sides(back_buffer,black);
	}
	draw_score(pscored,cscored,buffer,blue);

        /* wait for vblank before switching buffers */
        wait_vblank();

        /* swap the buffers */
        buffer = flip_buffers(buffer);
    }
}

/* the game boy advance uses "interrupts" to handle certain situations
 * for now we will ignore these */
void interrupt_ignore() {
    /* do nothing */
}

/* this table specifies which interrupts we handle which way
 * for now, we ignore all of them */
typedef void (*intrp)();
const intrp IntrTable[13] = {
    interrupt_ignore,   /* V Blank interrupt */
    interrupt_ignore,   /* H Blank interrupt */
    interrupt_ignore,   /* V Counter interrupt */
    interrupt_ignore,   /* Timer 0 interrupt */
    interrupt_ignore,   /* Timer 1 interrupt */
    interrupt_ignore,   /* Timer 2 interrupt */
    interrupt_ignore,   /* Timer 3 interrupt */
    interrupt_ignore,   /* Serial communication interrupt */
    interrupt_ignore,   /* DMA 0 interrupt */
    interrupt_ignore,   /* DMA 1 interrupt */
    interrupt_ignore,   /* DMA 2 interrupt */
    interrupt_ignore,   /* DMA 3 interrupt */
    interrupt_ignore,   /* Key interrupt */
};
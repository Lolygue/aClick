#define VERSION "1.0.0"
#ifdef DEBUG
#define VERSION "DEBUG 1.0.0"
#endif

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

#define Font X11Font
#include <raylib.h>

#define WINWIDTH 320
#define WINHEIGHT 130
#define SIZEOF_INPBUF 16
#define KEYHOLD_LENGTH 200
#define UNSET INT32_MIN

typedef struct option_s {
	char* name;
	char* format;
	char* tip;
	double* variable;
	double min;
	double max;
	double step;
	bool integer;
} option_t;

Display* display;
int nOptions;
double interval = 100;
double button = 1;
option_t* options;
sem_t clickSem;

void createOption(char* name, char* format, char* tip, double* variable, double min, double max, double step, bool integer){
	option_t* newOptions = realloc(options, sizeof(option_t) * (nOptions+1));
	if(!newOptions){
		fputs("Failed to reallocate memory", stderr);
	}else{
		options = newOptions;
		options[nOptions] = (option_t){
			.name = name,
			.format = format,
			.tip = tip,
			.variable = variable,
			.min = min,
			.max = max,
			.step = step
		};
		nOptions++;
	}
}

long long getTimeMs(){
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	long long now = ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
	return now;
}

void click(){
	XTestFakeButtonEvent(display, button, True, CurrentTime);
	XFlush(display);

	XTestFakeButtonEvent(display, button, False, CurrentTime);
	XFlush(display);
}

void* clickThread(void* args){
	while(true){
		sem_wait(&clickSem);
		long long first = getTimeMs();
		click();
		sem_post(&clickSem);
		while(getTimeMs() - first < interval);
	}
	return NULL;
}

int main(int argc, char** argv){
	options = malloc(sizeof(option_t));
	if(!options){
		fputs("Failed to allocate memory", stderr);
		return 1;
	}
	nOptions = 0;
	int selIdx = 0;

	display = XOpenDisplay(NULL);
	if(!display){
		fputs("Failed to open X11 Display", stderr);
		return 1;
	}

	createOption(
		"Interval: %sms",
		"%.1f",
		"Interval between clicks.",
		&interval, 0.1, UNSET, 1.0,
		false
	);
	createOption(
		"Button: %s",
		"%.0f",
		"Click button.\n1=LMB 2=MMB 3=RMB 4=SUP 5=SDOWN",
		&button, 1, 5, 1,
		true
	);

	InitWindow(WINWIDTH, WINHEIGHT, "aClick");
	SetExitKey(KEY_NULL);
	SetTargetFPS(30);

	XEvent event;
	Window root = DefaultRootWindow(display);
	KeyCode keycodeF8 = XKeysymToKeycode(display, XK_F8);
	XGrabKey(display, keycodeF8, 0, root, True, GrabModeAsync, GrabModeAsync);
	XSelectInput(display, root, KeyPressMask);
	XSync(display, False);

	pthread_t thread = (pthread_t)NULL;
	sem_init(&clickSem, 0, 1);
	
	bool clicking;
	bool typing = false;
	long long pressTime;
	char inpBuf[SIZEOF_INPBUF+1];
	char* inpBufp = inpBuf;
	while(!WindowShouldClose()){
		while(XPending(display)){
			XNextEvent(display, &event);
			if(event.type == KeyPress){
				if((clicking = !clicking)){
					pthread_create(&thread, NULL, clickThread, NULL);
				}else{
					sem_wait(&clickSem);
					pthread_cancel(thread);
					sem_post(&clickSem);
				}
			}
		}

		if(IsKeyPressed(KEY_DOWN)){
			selIdx++;
			selIdx %= nOptions;
		}
		if(IsKeyPressed(KEY_UP)){
			selIdx--;
			if(selIdx < 0) selIdx = nOptions-1;
		}

		option_t currentOption = options[selIdx];
		if(IsKeyDown(KEY_LEFT)){
			long long now = getTimeMs();
			if(IsKeyPressed(KEY_LEFT)) pressTime = now;

			if(pressTime == now || now - pressTime > KEYHOLD_LENGTH){
				*currentOption.variable -= (currentOption.step * (IsKeyDown(KEY_LEFT_SHIFT) ? 10.0 : 1.0));
				if(*currentOption.variable < currentOption.min && currentOption.min != UNSET){
					*currentOption.variable = currentOption.max == UNSET ? currentOption.min : currentOption.max;
				}
			}
		}else if(IsKeyDown(KEY_RIGHT)){
			long long now = getTimeMs();
			if(IsKeyPressed(KEY_RIGHT)) pressTime = now;

			if(pressTime == now || now - pressTime > KEYHOLD_LENGTH){
				*currentOption.variable += (currentOption.step * (IsKeyDown(KEY_LEFT_SHIFT) ? 10.0 : 1.0));
				if(*currentOption.variable > currentOption.max && currentOption.max != UNSET){
					*currentOption.variable = currentOption.min == UNSET ? currentOption.max : currentOption.min;
				}
			}
		}

		int key;
		char* endptr;
		double newValue;
		while((key = GetKeyPressed()) != 0){
			switch(key){
				case KEY_KP_0...KEY_KP_9:
					key -= (KEY_KP_0 - KEY_ZERO);
				case KEY_ZERO...KEY_NINE:
				case KEY_PERIOD:
					typing = true;
					if(inpBufp - SIZEOF_INPBUF != inpBuf){
						*inpBufp = key;
						inpBufp++;
						*inpBufp = '\0';
					}
					break;
					
				case KEY_BACKSPACE:
					typing = true;
					if(inpBufp != inpBuf){
						inpBufp--;
						*inpBufp = '\0';
					}
					break;

				case KEY_ENTER:
					if(!typing){
						typing = true;
						break;
					}
					newValue = strtod(inpBuf, &endptr);
					if(endptr == inpBufp){
						if(currentOption.integer) newValue = (int)newValue;
						if(newValue > currentOption.max && currentOption.max != UNSET){
							newValue = currentOption.max;
						}else if(newValue < currentOption.min && currentOption.min != UNSET){
							newValue = currentOption.min;
						}
						*currentOption.variable = newValue;
					}
				case KEY_RIGHT...KEY_UP:
				case KEY_ESCAPE:
					typing = false;
					inpBufp = inpBuf;
					*inpBufp = '\0';
					break;
			}
		}

		BeginDrawing();

			ClearBackground(BLACK);
			DrawText("aClick", 10, 10, 20, GRAY);
			DrawText(VERSION, 80, 18, 10, GRAY);
			DrawText(TextFormat("[F8] clicking: %s", clicking ? "true" : "false"), 10,40,10, clicking ? GREEN : RED);

			if(!typing) DrawText("*", 10, 60 + selIdx*10, 10, WHITE);
			for(int i=0; i<nOptions; i++){
				option_t option = options[i];
				const char* formatted = TextFormat(
					option.name, (typing && selIdx == i) ?
					inpBuf :
					TextFormat(option.format, *option.variable)
				);
				DrawText(formatted, 20,60 + i*10,10, selIdx == i ? WHITE : GRAY);
			}
			DrawText(currentOption.tip, 10, WINHEIGHT-21, 10, WHITE);

		EndDrawing();
	}

	sem_destroy(&clickSem);
	if(thread != (pthread_t)NULL) pthread_cancel(thread);
	XUngrabKey(display, keycodeF8, 0, root);
	XCloseDisplay(display);
	free(options);
	CloseWindow();

	return 0;
}
/*
 * See Licensing and Copyright notice in naev.h
 */


/**
 * @file intro.c
 *
 * @brief Handles the introduction sequence.
 *
 * @todo Allow handling of images and other fancy things once we get them.
 */


/** @cond */
#include "SDL.h"

#include "naev.h"
/** @endcond */

#include "intro.h"

#include "conf.h"
#include "font.h"
#include "log.h"
#include "music.h"
#include "ndata.h"
#include "nstring.h"
#include "toolkit.h"


#define INTRO_SPEED        30. /**< Speed of text in characters / second. */
#define SIDE_MARGIN       100. /**< Minimum space on either side of the text. */
#define IMAGE_WIDTH       300. /**< Width to reserve for images on the side. */


/**
 * @brief Intro Image: to be displayed to the side of the scrolling.
 */
typedef struct intro_img_t_ {
   glTexture *tex;     /* Image. */
   double x, y, w, h;  /* Position. */
   glColour c;         /* Alpha channel for fading. */
   double fade_rate;   /* Positive for fade-in, negative for fade-out. */
} intro_img_t;

/*
 * Introduction text lines.
 */
static char **intro_lines = NULL;  /**< Introduction text lines. */
static int intro_nlines = 0;       /**< Number of introduction text lines. */
static glFont intro_font;          /**< Introduction font. */

static int has_side_gfx = 0;       /* Determines how wide to make the text. */

/*
 * Prototypes.
 */
static int intro_load( const char *text );
static void intro_cleanup (void);
static void intro_event_handler( int *stop, double *offset, double *vel );
static void initialize_image( intro_img_t *img );
static void load_image( intro_img_t *img, const char *img_file );
static void intro_fade_image_in( intro_img_t *side, intro_img_t *transition,
                                 const char *img_file );
static int  intro_draw_text( char **const sb_list, int sb_size, int sb_index, double offset, double line_height );


/**
 * @brief Loads the intro stuff.
 */
static int intro_load( const char *text )
{
   size_t intro_size;
   char *intro_buf;
   const char *cur_line;
   char *rest_of_file;
   char img_src[STRMAX_SHORT];     /* path to image to be displayed alongside text. */
   int length;
   int i, n;
   int mem;

   has_side_gfx = 0;

   /* Load text. */
   intro_buf = ndata_read( text, &intro_size );

   /* Create intro font. */
   gl_fontInit( &intro_font, FONT_MONOSPACE_PATH, conf.font_size_intro, FONT_PATH_PREFIX, 0 );

   /* Accumulate text into intro_lines. At each step, keep track of:
    * * cur_line: text to go from the current input line, after word-wrapping.
    * * rest_of_file: pointer to just after the current input line (or NULL if we're on the last one).
    * * n: output line #
    * * i: current line length (strlen(cur_line), or less if we must word-wrap).
    * We can start in a state with no word-wrap spill-over and "rest_of_file" pointing to the full input.
    */
   n = 0;
   mem = 0;
   cur_line = "";
   rest_of_file = intro_buf;
   while (1) {
      i = strlen(cur_line);
      /* Advance a line in the source file if necesary. */
      if (i == 0) {
         if (rest_of_file == NULL)
            break;
         cur_line = rest_of_file;
         rest_of_file = strchr(cur_line, '\n');
         /* If there's a next line, split the string and point rest_of_file to it. */
         if (rest_of_file != NULL) {
            /* Check for CRLF endings -- if present, zero both parts. */
            if (rest_of_file > cur_line && *(rest_of_file-1) == '\r')
               *(rest_of_file-1) = '\0';
            *rest_of_file++ = '\0';
         }
         /* Translate if plain text (not empty, not a directive). */
         if (cur_line[0] != '\0' && cur_line[0] != '[')
            cur_line = _(cur_line);
         i = strlen(cur_line);
      }
      /* Copy the line. */
      if (n+1 > mem) {
         mem += 128;
         intro_lines = realloc( intro_lines, sizeof(char*) * mem );
      }

      if ( cur_line[0] == '[' /* Don't do sscanf for every line! */
           && sscanf( &cur_line[0], "[fadein %s", img_src ) == 1 ) {
         /* an image to appear next to text. */

         length = strlen( img_src );
         intro_lines[n] = malloc( length + 2 );
         intro_lines[n][0] = 'i';
         strncpy( &intro_lines[n][1], img_src, length + 1 );
         intro_lines[n][length] = '\0';

         /* Mark that there are graphics. */
         has_side_gfx = 1;

      } else if ( cur_line[0] == '[' /* Don't do strncmp for every line! */
           && strncmp( &cur_line[0], "[fadeout]", 9 ) == 0 ) {
         /* fade out the image next to the text. */

         intro_lines[n] = malloc( 2 );
         intro_lines[n][0] = 'o';
         intro_lines[n][1] = '\0';   /* not strictly necessary, but safe. */

      } else { /* plain old text. */
         /* Get the length. */
         i = gl_printWidthForText( &intro_font, cur_line, SCREEN_W - 2*SIDE_MARGIN - IMAGE_WIDTH, NULL );

         intro_lines[n] = malloc( i + 2 );
         intro_lines[n][0] = 't';
         strncpy( &intro_lines[n][1], cur_line, i );
         intro_lines[n][i+1] = '\0';
      }

      cur_line += i; /* Move pointer. */
      n++; /* New line. */
   }

   /* Clean up. */
   free(intro_buf);

   intro_nlines = n;
   return 0;
}


/**
 * @brief Cleans up the intro stuff.
 */
static void intro_cleanup (void)
{
   int i;

   /* Free memory. */
   for (i=0; i<intro_nlines; i++)
      free(intro_lines[i]);
   free(intro_lines);
   gl_freeFont(&intro_font);

   /* Set defaults. */
   intro_lines = NULL;
   intro_nlines = 0;
}

/**
 * @brief Initialize an intro_img_t to default values.
 *
 *    @brief img Image to initialize.
 */
static void initialize_image( intro_img_t *img )
{
   img->tex = NULL;
   img->c.r = 1.0;
   img->c.g = 1.0;
   img->c.b = 1.0;
   img->c.a = 1.0;
   img->fade_rate = 0.0;
}


/**
 * @brief Initialize an intro_img_t to default values.
 *
 *    @brief img Image to initialize.
 *    @brief img_file Path to the image file on disk.
 */
static void load_image( intro_img_t *img, const char *img_file )
{
      img->tex = gl_newImage( img_file, 0 );
      img->w = MIN( img->tex->w, IMAGE_WIDTH );
      img->h = img->tex->h * img->w / img->tex->w;
      img->x = (IMAGE_WIDTH + SIDE_MARGIN - img->w) / 2.0;
      img->y = (double)SCREEN_H / 2.0 - (img->h / 2.0);
      img->c.a = 0.0;
      img->fade_rate = 0.1;
}


/**
 * @brief Fade an image in.
 *
 *    @brief side Present image being displayed.
 *    @brief transition Image in transition or on deck.
 *    @brief img_file Path to the image file on disk.
 */
static void intro_fade_image_in( intro_img_t *side, intro_img_t *transition,
                                 const char *img_file )
{
   if (NULL == side->tex)
      load_image( side, img_file ); /* Simple fade-in. */
   else {
      /*
       * Transition or on-deck. The difference is whether one image is
       * replacing the other (transition), or whether the first must fade out
       * completely before the second comes in (on-deck).
       *
       * We can determine which is the case by seeing whether [fadeout] has been
       * called. I.e., is side->fade_rate < 0?
       */
      if (NULL != transition->tex) {
         /* Scrolling is happening faster than fading... */
         WARN( _("Intro scrolling too fast!") );
         gl_freeTexture( transition->tex );
      }
      load_image( transition, img_file );
      if (side->fade_rate < 0.0)
         transition->fade_rate = 0.0; /* put an image on deck. */
      else {
         /* transition. */
         side->fade_rate = -0.1; /* begin fading out. */
         side->c.a = 0.99;
      }
   }
}


/**
 * @brief Handle user events (mouse clicks, key presses, etc.).
 *
 *    @brief stop Whether to stop the intro.
 *    @brief vel How fast the text should scroll.
 */
static void intro_event_handler( int *stop, double *offset, double *vel )
{
   SDL_Event event;           /* user key-press, mouse-push, etc. */

   while (SDL_PollEvent(&event)) {
      if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_RESIZED) {
         naev_resize();
         continue;
      }

      if (event.type != SDL_KEYDOWN)
         continue;

      /* Escape skips directly. */
      if (event.key.keysym.sym == SDLK_ESCAPE) {
         *stop = 1;
         break;
      }

      /* Slow down the text. */
      else if (event.key.keysym.sym == SDLK_UP) {
         *vel -= 8.0;
         if (*vel < 0.0)
            *vel = 0.0;
      }

      /* Speed up the text. */
      else if (event.key.keysym.sym == SDLK_DOWN) {
         *vel += 8.0;
         if (*vel > 100.0)
            *vel = 100.0;
      }

      /* Jump down. */
      else if ((event.key.keysym.sym == SDLK_SPACE) ||
            (event.key.keysym.sym == SDLK_RETURN))
         *offset += 100;

      /* Jump up. */
      else if (event.key.keysym.sym == SDLK_BACKSPACE)
         *offset -= 100;

      /* User is clearly flailing on keyboard. */
      else
         *vel = 16.;
   }
}

/**
 * @brief Draw intro text onto the screen.
 *
 *    @brief sb_list List of text lines.
 *    @brief offset For smooth scrolling.
 *    @brief line_height V-space of the font (plus leading).
 *    @return Whether to stop.  1 if no text was rendered, 0 otherwise.
 */
static int intro_draw_text( char **const sb_list, int sb_size, int sb_index, double offset, double line_height )
{
   double       x, y; /* render position. */
   int          i;
   register int stop = 1;

   x = SIDE_MARGIN;
   if (has_side_gfx)
      x += IMAGE_WIDTH;

   i = sb_index;
   y = SCREEN_H + offset - line_height;
   do {
      if ( sb_list[ i ] != NULL ) {
         stop = 0;
         gl_printRaw( &intro_font, x, y, &cFontGreen, -1, sb_list[ i ] );
      }

      y -= line_height;
      i = ( i + 1 ) % sb_size;
   } while ( i != sb_index );

   return stop;
}

/**
 * @brief Displays the introduction sequence.
 *
 *    @brief text Path of text file to use.
 *    @brief mus Type of music to use (run through music.lua).
 *    @return 0 on success.
 */
int intro_display( const char *text, const char *mus )
{
   double       offset;           /* distance from bottom of the top line. */
   double       line_height;      /* # pixels per line. */
   int          lines_per_screen; /* max appearing lines on the screen. */
   char **      sb_arr;           /* array of lines to render. */
   int          sb_index;         /* Position in the line array. */
   double       vel  = 16.;       /* velocity: speed of text. */
   int          stop = 0;         /* stop the intro. */
   unsigned int tcur, tlast;      /* timers. */
   double       delta;            /* time diff from last render to this one. */
   int          line_index = 0;   /* index into the big list of intro lines. */
   intro_img_t  side_image;       /* image to go along with the text. */
   intro_img_t  transition;       /* image for transitioning. */

   /* Load the introduction. */
   if (intro_load(text) < 0)
      return -1;

   /* Change music to intro music. */
   if (mus != NULL)
      music_choose(mus);

   /* We need to clear key repeat to avoid infinite loops. */
   toolkit_clearKey();

   /* Do a few calculations to figure out how many lines can be present on the
      screen at any given time. */
   line_height = (double)intro_font.h * 1.3;
   lines_per_screen = (int)(SCREEN_H / line_height + 1.5); /* round up + 1 */

   sb_arr   = calloc( lines_per_screen, sizeof( char * ) );
   sb_index = 0;

   /* Force the first line to be loaded immediately. */
   offset = line_height;

   /* Unset the side image. */
   initialize_image( &side_image );
   initialize_image( &transition );

   tlast = SDL_GetTicks();
   while (!stop) {
      tcur = SDL_GetTicks();
      delta = (double)(tcur - tlast) / 1000.;
      tlast = tcur;

      /* Increment position. */
      offset += vel * delta;
      while (! (offset < line_height)) {
         /* One line has scrolled off, and another one on. */

         if (line_index < intro_nlines) {
            switch (intro_lines[line_index][0]) {
            case 't': /* plain ol' text. */
               sb_arr[ sb_index ] = &intro_lines[ line_index ][ 1 ];
               offset -= line_height;
               sb_index = ( sb_index + 1 ) % lines_per_screen;
               break;
            case 'i': /* fade in image. */
               intro_fade_image_in( &side_image, &transition,
                                    &intro_lines[line_index][1] );
               break;
            case 'o': /* fade out image. */
               if (NULL == side_image.tex) {
                  WARN(_("Tried to fade out without an image.") );
                  break;
               }
               side_image.fade_rate = -0.1;
               side_image.c.a = 0.99;
               break;
            default:  /* unknown. */
               break;
            }
            ++line_index;
         } else {
            sb_arr[ sb_index ] = NULL;
            offset -= line_height;
            sb_index = ( sb_index + 1 ) % lines_per_screen;
         }
      } /* while (offset > line_height) */

      /* Fade the side image. */
      if (side_image.tex != NULL && side_image.c.a < 1.0) {
         side_image.c.a += delta * vel * side_image.fade_rate;

         if (transition.tex != NULL && transition.fade_rate > 0.0)
            transition.c.a += delta * vel * transition.fade_rate;

         if (side_image.c.a > 1.0) {
            /* Faded in... */
            side_image.c.a = 1.0;
            side_image.fade_rate = 0.0;
         }
         else if (side_image.c.a < 0.0) {
            /* Faded out... */
            gl_freeTexture( side_image.tex );
            if (transition.tex != NULL) {
               side_image.tex = transition.tex;
               side_image.c.a = transition.c.a;
               side_image.w   = transition.w;
               side_image.h   = transition.h;
               side_image.y   = transition.y;
               side_image.fade_rate = 0.1;
               transition.tex = NULL;
               transition.c.a = 1.0;
            }
            else {
               side_image.c.a = 1.0;
               side_image.tex = NULL;
               side_image.fade_rate = 0.0;
            }
         }
      }

      /* Clear stuff. */
      glClear(GL_COLOR_BUFFER_BIT);

      /* Only thing we actually care about updating is music. */
      music_update( 0. );

      /* Draw text. */
      stop = intro_draw_text( sb_arr, lines_per_screen, sb_index, offset, line_height );

      if (NULL != side_image.tex)
         /* Draw the image next to the text. */
         gl_blitScale( side_image.tex, side_image.x, side_image.y,
                       side_image.w, side_image.h, &side_image.c );

      if (NULL != transition.tex && transition.c.a > 0.0)
         /* Draw the image in transition. */
         gl_blitScale( transition.tex, transition.x, transition.y,
                       transition.w, transition.h, &transition.c );

      /* Display stuff. */
      SDL_GL_SwapWindow( gl_screen.window );


      SDL_Delay(10); /* No need to burn CPU. */

      /* Handle user events. */
      intro_event_handler( &stop, &offset, &vel );

   } /* while (!stop) */

   /* free malloc'd memory. */
   free( sb_arr );
   gl_freeTexture( side_image.tex );
   gl_freeTexture( transition.tex );

   /* Stop music, normal music will start shortly after. */
   music_stop();

   /* Clean up after the introduction. */
   intro_cleanup();

   return 0;
}

/*
 * vncfunc.h: VNC Client functions to be used for the graphical VNC console of libvirt-php
 *
 * See COPYING for the license of this software
 */

#ifndef __VNCFUNC_H__
# define __VNCFUNC_H__

int vnc_get_bitmap(char *server,
                   char *port,
                   char *fn);

int vnc_get_dimensions(char *server,
                       char *port,
                       int *width,
                       int *height);

int vnc_refresh_screen(char *server,
                       char *port,
                       int scancode);

int vnc_send_keys(char *server,
                  char *port,
                  char *keys);

int vnc_send_pointer_event(char *server,
                           char *port,
                           int pos_x,
                           int pos_y,
                           int clicked,
                           int release);

#endif /* __VNCFUNC_H__ */

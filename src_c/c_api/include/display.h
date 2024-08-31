#pragma once

Surface *display_set_mode_size_flags_depth_vsync(int width, int height, int flags, int depth, int vsync);
Surface *display_set_mode_size_flags_depth(int width, int height, int flags, int depth);
Surface *display_set_mode_size_flags(int width, int height, int flags);
Surface *display_set_mode_size(int width, int height);
Surface *display_set_mode();
int display_flip();
int display_update_rectlist(SDL_Rect *rect_list, int rect_count);
int display_update_rect(SDL_Rect *rect);
int display_update();

# ----------------------------
# Makefile Options
# ----------------------------

NAME = ship
ICON = icon.png
DESCRIPTION = "basic jank shooter"
COMPRESSED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)

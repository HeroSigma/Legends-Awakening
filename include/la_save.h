#ifndef GUARD_LA_SAVE_H
#define GUARD_LA_SAVE_H

// Serialized layout v1: original LA prefix, Field Log, fake RTC, then this tag.
#define LA_SAVE_LAYOUT_V1 0x3153414C // "LAS1"

void LaSaveBlock3OnLoad(void);

#endif

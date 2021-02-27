#ifndef INC_LOG_H
#define INC_LOG_H

#include "fs.h"

struct buf;

void initlog(int dev, struct superblock *);
void log_write(struct buf *);
void begin_op();
void end_op();

#endif

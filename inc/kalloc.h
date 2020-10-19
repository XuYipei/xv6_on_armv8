#ifndef KERN_KALLOC_H
#define KERN_KALLOC_H

void alloc_init();
char *kalloc();
void kfree(char*);
void free_range(void *, void *);
void check_free_list();

char *balloc(uint64_t);
void bfree(void *, uint64_t);

#endif /* !KERN_KALLOC_H */

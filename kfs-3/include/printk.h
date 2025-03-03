#ifndef _PRINTK_H
#define _PRINTK_H 

#define KERN_ERR	"<0>"
#define KERN_INFO 	"<1>"

#define HAS_KERN_LOG_LEVEL(format) \
    ((format)[0] == '<' && (format)[1] >= '0' && (format)[1] <= '1' && (format)[2] == '>')

void printk(const char *, ...);

#endif

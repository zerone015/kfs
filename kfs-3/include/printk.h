#ifndef _PRINTK_H
#define _PRINTK_H 

#define KERN_ERR	    "<0>"
#define KERN_INFO 	    "<1>"
#define KERN_DEBUG 	    "<2>"

extern void printk(const char *__restrict, ...);

#define HAS_LOG_LEVEL(format) \
    ((format)[0] == '<' && (format)[1] >= '0' && (format)[1] <= '2' && (format)[2] == '>')

#endif

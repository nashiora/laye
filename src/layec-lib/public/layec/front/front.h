#ifndef LAYEC_FRONT_FRONT_H
#define LAYEC_FRONT_FRONT_H

typedef enum layec_front_end_status
{
    LAYEC_FRONT_SUCCESS = 0,
} layec_front_end_status;

typedef layec_front_end_status (*layec_front_end_entry_function)();

#endif // LAYEC_FRONT_FRONT_H

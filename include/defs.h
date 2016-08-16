#pragma once

#include <err.h>
#include <stdlib.h>

#define MAX_EVENTS 10
#define BUFFER_SIZE 1000

enum class FDType {
    events,
    listen,
    client
};

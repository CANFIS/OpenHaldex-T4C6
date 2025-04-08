#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#ifdef ENABLE_DEBUG
#define DEBUG(x, ...) printf(x "\n", ##__VA_ARGS__)
#define DEBUG_(x, ...) printf(x, ##__VA_ARGS__)
#else
#define DEBUG(x, ...)
#define DEBUG_(x, ...)
#endif

#define ARRAYSIZE(a) ((sizeof(a) / sizeof(*(a))) / static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))
#define portMAX_EVENT_BITS (portMAX_DELAY & (~0xff000000UL))

enum openhaldex_mode_t
{
  MODE_STOCK,
  MODE_FWD,
  MODE_5050,
  MODE_7525,
  MODE_CUSTOM,
  openhaldex_mode_t_MAX
};

const char *get_openhaldex_mode_string(openhaldex_mode_t mode)
{
  switch (mode)
  {
    case MODE_STOCK:
      return "STOCK";
    case MODE_FWD:
      return "FWD";
    case MODE_5050:
      return "5050";
    case MODE_7525:
      return "7525";
    case MODE_CUSTOM:
      return "CUSTOM";
    default:
      break;
  }
  return "?";
}

#endif

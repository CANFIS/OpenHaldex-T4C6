#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

void close_file(FILE **fpp)
{
  // If the double-pointer and the pointer are valid, close the file and set the pointer to NULL.
  if (fpp && *fpp)
  {
    fclose(*fpp);
    *fpp = NULL;
  }
}

bool open_file(FILE **fpp, const char *filename, const char *mode)
{
  // If the pointer is valid, open the file and store its file descriptor there.
  if (fpp)
  {
    close_file(fpp);
    *fpp = fopen(filename, mode);
    return (*fpp != NULL);
  }

  return false;
}

// Read a BYTE from the given file.
uint16_t read8(FILE *f)
{
  uint8_t result;
  fread(&result, sizeof(result), 1, f);
  return result;
}

// Read a WORD from the given file.
uint16_t read16(FILE *f)
{
  uint16_t result;
  fread(&result, sizeof(result), 1, f);
  return result;
}

// Read a DWORD from the given file.
uint32_t read32(FILE *f)
{
  uint32_t result;
  fread(&result, sizeof(result), 1, f);
  return result;
}

// Read a QWORD from the given file.
uint64_t read64(FILE *f)
{
  uint64_t result;
  fread(&result, sizeof(result), 1, f);
  return result;
}

// Read a Float32 from the given file.
float readFloat32(FILE *f)
{
  float result;
  fread(&result, sizeof(result), 1, f);
  return result;
}

// Read a string from the given file.
bool readCharArray(FILE *f, char *buf, size_t buf_len)
{
  bool ret = true;
  size_t idx = 0;

  if (f)
  {
    int get_c;
    while ((get_c = fgetc(f)))
    {
      if (get_c == EOF)
      {
        if (buf)
        {
          buf[0] = 0;
        }
        return false;
      }

      char c = (char)get_c;

      if (buf && (idx < buf_len))
      {
        buf[idx] = c;
      }
      else
      {
        ret = false;
      }

      idx++;
    }

    if (buf)
    {
      if (idx < buf_len)
      {
        buf[idx] = 0;
      }
      else
      {
        buf[buf_len - 1] = 0;
      }
    }
  }

  return ret;
}

// Write a BYTE to the given file.
void write8(FILE *f, uint8_t value)
{
  fwrite(&value, sizeof(value), 1, f);
}

// Write a WORD to the given file.
void write16(FILE *f, uint16_t value)
{
  fwrite(&value, sizeof(value), 1, f);
}

// Write a DWORD to the given file.
void write32(FILE *f, uint32_t value)
{
  fwrite(&value, sizeof(value), 1, f);
}

// Write a QWORD to the given file.
void write64(FILE *f, uint64_t value)
{
  fwrite(&value, sizeof(value), 1, f);
}

// Write a Float32 to the given file.
void writeFloat32(FILE *f, float value)
{
  fwrite(&value, sizeof(value), 1, f);
}

// Write a string to the given file.
void writeCharArray(FILE *f, char *str)
{
  fwrite(str, sizeof(char), strlen(str) + 1, f);
}

#endif

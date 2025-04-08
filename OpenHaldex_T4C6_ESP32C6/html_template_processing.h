typedef char *(*template_processor_t)(const char *templated_string, size_t string_length, char *processed_string);
char file_scratch[8192];

esp_err_t httpd_resp_send_with_template_processing(httpd_req_t *r, const char *buf, ssize_t buf_len, template_processor_t processor)
{
    char *chunk = file_scratch;                     // temporary buffer to which characters from the input buffer are copied while searching for a template
    const size_t chunk_size = sizeof(file_scratch); // length of the temporary buffer
    size_t chunk_index = 0;                         // index for the temporary buffer
    size_t input_buffer_index = 0;                  // index for the given buffer

    // Search the given buffer.
    while (input_buffer_index < buf_len)
    {
        // If the current character is an opening curly brace, check if the next character is as well, and also handle an escape character.
        if (buf[input_buffer_index] == '{')
        {
            // If there isn't enough room in the temporary buffer for copying in the next three characters, send what we have so far.
            if (chunk_index >= chunk_size - 3)
            {
                esp_err_t ret;
                if ((ret = httpd_resp_send_chunk(r, chunk, chunk_index)))
                {
                    return ret;
                }
                chunk_index = 0;
            }

            // Copy the 3 characters, which should be two opening curly braces and the first character of the template string (or an escape).
            chunk[chunk_index++] = buf[input_buffer_index++];
            chunk[chunk_index++] = buf[input_buffer_index++];
            chunk[chunk_index++] = buf[input_buffer_index];

            // Check if we have the two opening curly braces.
            if (chunk_index >= 3 && chunk[chunk_index - 2] == '{' && chunk[chunk_index - 3] == '{')
            {
                // If the third character is an escape (backslash), decrement the index (deleting the character).
                if (chunk[chunk_index - 1] == '\\')
                {
                    chunk_index--;
                }

                // If the third character is the beginning of a template string, parse it.
                else
                {
                    // If we have other characters in the buffer apart from our 3, send them.
                    if (chunk_index > 3)
                    {
                        esp_err_t ret;
                        if ((ret = httpd_resp_send_chunk(r, chunk, chunk_index - 3)))
                        {
                            return ret;
                        }
                    }

                    // Put the first character of the template string on the first position in the temporary buffer.
                    chunk[0] = buf[input_buffer_index++];
                    chunk_index = 1;

                    // Search for the two closing curly braces.
                    bool found = false;
                    char last_char = 0;
                    while (!found && (chunk_index < chunk_size - 1))
                    {
                        char ch = buf[input_buffer_index++];

                        if (ch == '}' && last_char == '}')
                        {
                            found = true;
                        }
                        else if (ch != '}')
                        {
                            chunk[chunk_index++] = ch;
                        }

                        last_char = ch;
                    }

                    // Decrement the index, as it was also incremented on the last step of the search.
                    input_buffer_index--;

                    // If the end of the temporary buffer was reached before finding the closing braces, return an error.
                    if (!found)
                    {
                        httpd_resp_sendstr(r, "Template parsing error");
                        httpd_resp_set_hdr(r, "Connection", "close");
                        httpd_resp_send_chunk(r, NULL, 0);
                        return ESP_FAIL;
                    }

                    static char processed_string[64];
                    char *processed = processor(chunk, chunk_index, processed_string);

                    esp_err_t ret;
                    if (!strlen(processed))
                    {
                        ret = httpd_resp_sendstr_chunk(r, " ");
                    }
                    else
                    {
                        ret = httpd_resp_sendstr_chunk(r, processed);
                    }

                    if (ret)
                    {
                        return ret;
                    }

                    chunk_index = 0;
                }
            }
        }
        else
        {
            chunk[chunk_index++] = buf[input_buffer_index];

            if (chunk_index >= chunk_size - 1)
            {
                esp_err_t ret;
                if ((ret = httpd_resp_send_chunk(r, chunk, chunk_index)))
                {
                    return ret;
                }
                chunk_index = 0;
            }
        }

        input_buffer_index++;
    }

    // Send the last part.
    if (chunk_index)
    {
        esp_err_t ret;
        if ((ret = httpd_resp_send_chunk(r, chunk, chunk_index)))
        {
            return ret;
        }
    }

    // Send empty chunk to signal HTTP response completion.
    httpd_resp_set_hdr(r, "Connection", "close");
    httpd_resp_send_chunk(r, NULL, 0);
    return ESP_OK;
}

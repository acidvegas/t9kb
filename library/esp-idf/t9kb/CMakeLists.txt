# T9KB - ESP-IDF component (ESP-IDF 5.2 or newer). The C core is shared with the Arduino library.
idf_component_register(
	SRCS         "t9kb_idf.c" "../../arduino/src/t9kb_core.c" "../../arduino/src/t9kb_dict.c"
	INCLUDE_DIRS "include" "../../arduino/src"
	REQUIRES     driver esp_timer
)

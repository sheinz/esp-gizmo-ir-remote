
INC_DIRS += $(esp-config_ROOT)

esp-config_SRC_DIR = $(esp-config_ROOT)

$(eval $(call component_compile_rules,esp-config))

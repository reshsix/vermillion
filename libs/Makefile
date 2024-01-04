# This file is part of vermillion.

# Vermillion is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published
# by the Free Software Foundation, version 3.

# Vermillion is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with vermillion. If not, see <https://www.gnu.org/licenses/>.

.PHONY: all image clean tools depclean debug test uart \
        config menuconfig xconfig defconfig

BUILD := $(BUILD)/libs/$(NAME)
OBJS := $(addprefix $(BUILD)/,$(OBJS))

all image: $(OBJS)
clean:
	@printf '%s\n' "  RM      build/libs/$(NAME)"
	@rm -rf $(BUILD)

$(BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	@printf '%s\n' "  CC      libs/$(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) $< -o $@

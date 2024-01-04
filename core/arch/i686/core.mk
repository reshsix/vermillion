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

# --------------------------------- Recipes  --------------------------------- #

# Helper recipes
image: $(BUILD)/vermillion.img
debug: $(BUILD)/vermillion.img scripts/debug.gdb
	@printf '%s\n' "  QEMU    $(<:$(BUILD)/%=%)"
	@qemu-system-i386 -s -S -cdrom $< &
	@gdb-multiarch --command=scripts/debug.gdb
test: $(BUILD)/vermillion.img
	@printf '%s\n' "  TEST    $(<:$(BUILD)/%=%)"
	@chronic sh -c "qemu-system-i386 -device isa-debug-exit -cdrom $< \
                    -nographic || [ \$$? = 255 ]"

# Specific recipes
$(BUILD)/boot.o: arch/$(ARCH)/boot.S deps/.$(TARGET)-gcc | $(BUILD)
	@printf '%s\n' "  AS      core/$(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/vermillion.img: $(BUILD)/kernel.elf \
                         arch/$(ARCH)/grub.cfg | $(BUILD)/mount
	@printf '%s\n' "  BUILD   $(@:$(BUILD)/%=%)"
	@mkdir -p $(BUILD)/mount/boot/grub
	@cp $< $(BUILD)/mount/boot/
	@cp arch/$(ARCH)/grub.cfg $(BUILD)/mount/boot/grub/
	@chronic grub-mkrescue -o $@ $(BUILD)/mount

# --------------------------------- Objects  ---------------------------------

OBJS += boot.o

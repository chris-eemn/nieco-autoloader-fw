# AUT-63 — Add cal-data for NV storage

Added a cal-data module that stores application settings and stepper positions on the SPI flash
so they survive a power cycle. It uses the upper 1 MB of the chip; the bootloader trigger and
staged firmware image keep the lower half. Storage is split into sections rather than one blob:
general settings in one, and each of the four stepper pairs in its own. That split matters
because flash erases in 4 KB chunks, so anything sharing a chunk with a frequently-written value
gets wiped and rewritten every time that value changes — this way a position save touches nothing
but that pair's data. Positions are stored as encoder counts, not commanded steps, since
commanded steps drift the moment a motor stalls. Each position is written twice and compared on
read; a mismatch means the save was interrupted, so the position reports as unknown and the axis
re-homes. Nothing consumes the stored values yet beyond loading them at boot.

`w25q_config.h` now holds the authoritative chip address map (trigger flag at `0x000000`, staged
image from `0x001000`, cal-data at `0x100000`), and `update_image.h` and `btl_flash_trigger.c`
derive from it instead of hard-coding offsets. `spi_flash_io.c/.h` moved from `usart3_loader/` to
a new `common/` folder, since the loader, the bootloader trigger, and cal-data all share it.

The module is in `cal-data/`. `cal_data_map.h` owns the section layout — general section at
`0x100000` (4 sectors reserved), one sector per pair at `0x104000`–`0x107000` — with sections
over-allocated so growing a struct does not shift the ones behind it and orphan field data.
`cal_data.c` validates on magic plus layout version, keeps a RAM copy, and self-provisions a
blank chip with defaults that mirror the values currently hard-coded in `main.c`, so nothing
changes behaviour. Its writes keep `spi_flash_io_write()`'s read-back verification.
`cal_data_position.c` is the fast path and uses `w25q_queue_command()` instead: it stages both
copies in a per-pair static buffer (the driver keeps the caller's pointer and writes later from
the SPI ISR) and queues an erase plus a write, returning immediately rather than stalling the
motor task ~45 ms on the erase. Capacity is checked before either command is pushed, so an erase
is never queued without its write. Trade-off accepted: queued writes are not verified, and a
silent failure shows up as a failed two-copy compare, i.e. a re-home. Deviations from the
reference `caldata.c`, all agreed first: no payload CRC, no FreeRTOS mutex (single-task), and
async writes for positions. Builds clean, no new warnings.

Follow-ups:

- Nothing reads the parameters yet; feeding `axis_config_t` and `stepper_ctrl` from cal-data
  means editing existing init code.
- CLI untouched — `cli/cal_data_example.c` is still the standalone example store.
- Position saves still cost a sector erase; journaling within the sector would take a typical
  save to ~1 ms if the write rate turns out to be high.
- Unit tests deferred pending a host gcc toolchain.

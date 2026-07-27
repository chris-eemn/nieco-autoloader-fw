## Requirements/notes
 - The goal is to add a cal-data module to this stepper project to store/recall non-volatile data.
 - even though the module is called cal-data, it doesnt necessarily have to be calibration data. It can be any data that needs to be stored in non-volatile memory and recalled later.
 - use w25q to store data. Use the w25q driver that is already in the project. The driver is in the drivers/w25q folder.
 - the w25q is used for multiple things, bootloading in the first part of the flash. Cal-data after. Ensure we leave enough space for the bootloading trigger flag + max size of stm32c5 firmware.
 - we probably need at least 2 sections. Stepper position will eventually get stored in nv storage. All other data can remain it its own section. The reason for this is the data lives in a struct to easily save it. We dont necesarrily want to be saving a huge strurct with all data when all we need to update is stepper position. It may lend that we have 5 secionds.
   - 1 section for normal data
   - 1 section for each stepper pair (which is not defined in this project yet, but will be in the future)
 - caldata.c/.h are example from another project. Id like to follow this example if we could. Any deviations i want you to discuss with me.
 - the w25q_config.h is generally where we store address section defines/documentation. I notice some of this has leaked out into update_image.h, update_image.h is fine but should just access the #defines created in w25q_config.h.
 - we dont really have the full app defined yet int his project. so lets just have some placeholder data + stepper position pairs. There will be 4 pairs of steppers (8 motors total)
    - some other non-position data that we can plan for now
      - pusher rpm
      - lifter rpm
      - home stall time (ms)
      - feel free to review the code and get back to me if you see more of this stuff we could store. I know we wont get everything in this pass which is okay
 - cal-data module should be stored in its own folder cal-data
 ## other considerations
  - ideally we are doing 0 refactoring to current code for this. if you see otherwise we need to discuss first
  - id like to see some units test for this. So the module should be abstracted away from the hw/app such that unit tests can run on the host system (not the target) via gcc or some such. If this is burdensome lets talk about it.

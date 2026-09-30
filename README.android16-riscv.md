# A16_RISCV/vendor/spacemit/hardware/sensors: android16-riscv

Changes made for the Android 16 (AOSP, riscv64) bring-up of the BananaPi BPI-F3 (SpacemiT K1) and the BananaPi BPI-SM10 (SpacemiT K3), on branch `android16-riscv`.

AIDL Sensors HAL for the BananaPi F3 header I2C4 modules.

## Changes

- **sensors: add a Sensors HAL for the BananaPi F3 header I2C4 modules**: AIDL Sensors HAL based on the AOSP example: GY-9250 IMU (MPU-925x/6500 accelerometer, gyroscope, AK8963 magnetometer) over i2c-dev, AHT20 temperature/humidity and BMP280 pressure through their kernel drivers; ro.vendor.sensors.imu.axes remaps a rotated module.

## Notes

- Enabled by `SPACEMIT_SENSORS` in device/spacemit/common/spacemit-features.mk. SELinux policy: device/spacemit/common/sepolicy.

## Build

```
source build/envsetup.sh
lunch aosp_bananapi_f3_tablet trunk_staging userdebug   # BPI-F3 (K1)
lunch aosp_bananapi_sm10_tablet trunk_staging userdebug # BPI-SM10 (K3), not booted yet
m android.hardware.sensors-service.spacemit
```

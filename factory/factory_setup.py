import argparse

from wintertools import fw_fetch, jlink, log

from libgemini import adc_calibration, ramp_calibration

DEVICE_NAME = "winterbloom_gemini"
JLINK_DEVICE = "ATSAMD21G18"
JLINK_SCRIPT = "scripts/flash.jlink"


def program_firmware():
    log.section("Programming firmware")

    fw_fetch.latest_bootloader(DEVICE_NAME)

    jlink.run(JLINK_DEVICE, JLINK_SCRIPT)


def run_ramp_calibration():
    log.section("Calibrating ramps")
    ramp_calibration.run(save=True)


def run_adc_calibration():
    log.section("Calibrating ADC")
    # TODO: copy over Sol's code.

    adc_calibration.run(
        num_calibration_points=50,
        sample_count=128,
        strategy="adc",
        save=True,
    )


def run_afe_calibration():
    log.section("Calibrating AFE")

    adc_calibration.run(
        num_calibration_points=50,
        sample_count=128,
        strategy="afe",
        save=True,
    )


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "--stages",
        type=str,
        nargs="*",
        default=["firmware", "ramp", "adc", "afe"],
        help="Select which setup stages to run.",
    )

    args = parser.parse_args()

    if "firmware" in args.stages:
        program_firmware()

    if "ramp" in args.stages:
        run_ramp_calibration()

    if "adc" in args.stages:
        run_adc_calibration()

    if "afe" in args.stages:
        run_afe_calibration()


if __name__ == "__main__":
    main()

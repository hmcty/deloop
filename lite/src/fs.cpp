#include "fs.hpp"

#include <daisy.h>
#include <ff.h>

#include "logging.hpp"
#include "metadata.hpp"
#include "time.hpp"

using namespace daisy;

const char *kMetadataPath = "/metadata.bin";

static struct {
  bool initialized;
  daisy::SdmmcHandler sdmmc;
  daisy::FatFSInterface fsi;
  uint8_t sd_buffer[_MAX_SS];
  FIL metadata_file;
  deloop::Metadata metadata;
} state_;

static bool create_fs(FATFS &fs);
static bool load_metadata(FATFS &fs);

void fs::init() {
  if (state_.initialized) return;

  SdmmcHandler::Config sdcfg;
  sdcfg.Defaults();
  sdcfg.speed = SdmmcHandler::Speed::STANDARD;
  sdcfg.width = SdmmcHandler::BusWidth::BITS_4;
  state_.sdmmc.Init(sdcfg);

  state_.fsi.Init(FatFSInterface::Config::MEDIA_SD);
  FATFS &fs = state_.fsi.GetSDFileSystem();

  DELOOP_LOG_INFO("Mounting filesystem...");
  FRESULT fr = f_mount(&fs, "/", 1 /* mount now */);
  if (fr != FR_OK) {
    if (fr == FR_NO_FILESYSTEM) {
      if (!create_fs(fs)) {
        // TODO: Report err
        return;
      }
    }
  }

  DELOOP_LOG_INFO("Loading metadata from %s", kMetadataPath);
  // if (!load_metadata(fs)) {
  //   DELOOP_LOG_ERROR("Failed to load metadata.");
  //   return;
  // } else {
  DELOOP_LOG_INFO("Metadata loaded successfully.");
  // }

  state_.initialized = true;
}

void fs::Tree(const char *path, size_t depth) {
  FILINFO fno;
  DIR dir;
  FRESULT fr;

  fr = f_opendir(&dir, path);
  if (fr != FR_OK) {
    return;
  }

  while (true) {
    fr = f_readdir(&dir, &fno);
    if (fr != FR_OK || fno.fname[0] == 0) {
      break;
    }

    DELOOP_LOG_INFO("%*s%s", depth * 2, "", fno.fname);
    if (fno.fattrib & AM_DIR) {
      char subpath[256];
      snprintf(subpath, sizeof(subpath), "%s/%s", path, fno.fname);
      fs::Tree(subpath, depth + 1);
    }
  }
}

deloop::Metadata &fs::GetMetadata() { return state_.metadata; }

static bool create_fs(FATFS &fs) {
  FRESULT fr =
      f_mkfs("/", FM_FAT32, 0, &state_.sd_buffer, sizeof(state_.sd_buffer));
  if (fr == FR_OK) {
    fr = f_mount(&fs, "/", 1 /* mount now */);
    return fr == FR_OK;
  } else {
    return false;
  }
}

static bool load_metadata(FATFS &fs) {
  bool already_exists = true;
  FRESULT fr = f_stat(kMetadataPath, nullptr);
  if (fr == FR_NO_FILE) {
    DELOOP_LOG_INFO("Metadata file does not exist. Creating new one.");
    already_exists = false;
  } else if (fr != FR_OK) {
    DELOOP_LOG_ERROR("Error checking metadata file: %d", fr);
    return false;
  }

  fr = f_open(&state_.metadata_file, kMetadataPath,
              FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
  if (fr != FR_OK) {
    DELOOP_LOG_ERROR("Error opening metadata file: %d", fr);
    return false;
  }

  UINT br;
  deloop::MetadataBuffer buffer;
  if (already_exists) {
    fr = f_read(&state_.metadata_file, buffer.data(), buffer.size(), &br);
    if (!deloop::ReadMetadata(state_.metadata, buffer)) {
      DELOOP_LOG_ERROR("Metadata is invalid.");
      f_close(&state_.metadata_file);
      return false;
    }
  } else {
    DELOOP_LOG_INFO("Creating new metadata file...");
    deloop::WriteMetadata(state_.metadata, buffer);
    fr = f_write(&state_.metadata_file, buffer.data(), buffer.size(), &br);
    fr = f_sync(&state_.metadata_file);
  }

  if (fr != FR_OK || br != buffer.size()) {
    DELOOP_LOG_ERROR("Error reading/writing metadata file: %d", fr);
    f_close(&state_.metadata_file);
    return false;
  }

  return true;
}

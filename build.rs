// build.rs

use std::process::Command;
use std::env;
use std::path::{Path, PathBuf};

fn main() {
    pkg_config::Config::new().probe("jack").unwrap();
    println!("cargo:rerun-if-changed=build.rs");

    let bindings = bindgen::Builder::default()
        .clang_arg("-Isrc")
        .header("src/bizzy/track.h")
        .header("src/bizzy/client.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");

    cc::Build::new()
        .include("src")
        .file("src/bizzy/client.c")
        .file("src/bizzy/track.c")
        .compile("bizzy"); 
    println!("cargo:rerun-if-changed=src/bizzy.c");
    println!("cargo:rustc-link-lib=jack");
}

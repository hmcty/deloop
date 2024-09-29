// build.rs

use std::env;
use std::path::PathBuf;

fn main() {
    pkg_config::Config::new().probe("jack").unwrap();
    println!("cargo:rerun-if-changed=build.rs");

    let bindings = bindgen::Builder::default()
        .clang_arg("-Isrc")
        .header("src/deloop/track.h")
        .header("src/deloop/client.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");

    cc::Build::new()
        .include("src")
        .file("src/deloop/client.c")
        .file("src/deloop/track.c")
        .file("src/deloop/logging.c")
        .compile("deloop");
    println!("cargo:rerun-if-changed=src/deloop.c");
    println!("cargo:rustc-link-lib=jack");
}

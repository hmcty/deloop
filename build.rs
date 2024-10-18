// build.rs

use std::env;
use std::path::PathBuf;

fn main() -> miette::Result<()> {
    pkg_config::Config::new().probe("jack").unwrap();
    println!("cargo:rerun-if-changed=build.rs");

    let path = std::path::PathBuf::from("src"); // include path
    // cxx_build::bridge("src/deloop/bridge.rs")

    let mut b = autocxx_build::Builder::new("src/main.rs", &[&path]).build()?;

    b.flag_if_supported("-std=c++20")
        .include("src")
        .file("src/deloop/client.cc")
        // .file("src/deloop/track.cc")
        // .file("src/deloop/logging.cc")
        .std("c++11")
        .compile("deloop");
    println!("cargo:rerun-if-changed=src/deloop/bridge.rs");
    println!("cargo:rerun-if-changed=src/deloop/client.cc");
    println!("cargo:rustc-link-lib=jack");

    Ok(())
}

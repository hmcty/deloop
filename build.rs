// build.rs

use std::process::Command;
use std::env;
use std::path::Path;

fn main() {
    pkg_config::Config::new().probe("jack").unwrap();
    println!("cargo:rerun-if-changed=build.rs");

    cc::Build::new()
        .file("src/bizzy.c")
        .compile("bizzy"); 
    println!("cargo:rerun-if-changed=src/bizzy.c");
    println!("cargo:rustc-link-lib=jack");
}

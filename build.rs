use cc::Build;
use glob::glob;
use pkg_config::Config;
use std::error::Error;

fn main() -> Result<(), Box<dyn Error>> {
    let mut build = Build::new();
    build.cpp(false).std("c17").include("csrc");
    for file in glob("csrc/**/*.c")? {
        let file = file?;
        build.file(&file);
        println!("cargo:rerun-if-changed={}", file.display());
    }
    build.compile("ibis_c");

    println!("cargo:rerun-if-changed=csrc/context.c");
    println!("cargo:rerun-if-changed=csrc/context.h");
    println!("cargo:rerun-if-changed=csrc/def.h");

    if cfg!(target_os = "windows") {
        println!("cargo:rustc-link-lib=vulkan-1");
        println!("cargo:rustc-link-lib=glfw3");
    } else {
        println!("cargo:rustc-link-lib=vulkan");
        Config::new().atleast_version("3.3").probe("glfw3").map_err(|error| {
            format!(
                "Failed to find GLFW ({error}). Install the GLFW development package (e.g. 'libglfw3-dev' on Debian/Ubuntu or 'glfw' via Homebrew)."
            )
        })?;
    }

    Ok(())
}

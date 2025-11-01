#![allow(clippy::missing_safety_doc)]

use naga::{
    back::wgsl,
    front::spv,
    valid::{Capabilities, ValidationFlags, Validator},
};
use std::{
    ffi::{CString, c_char},
    ptr, slice,
};

#[repr(C)]
pub struct ConvertResult {
    wgsl_string: *mut c_char,
    wgsl_length: usize,
    error_string: *mut c_char,
    error_length: usize,
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn convert_spirv_to_wgsl_alloc(
    spv: *const u8,
    spv_count: usize,
) -> ConvertResult {
    let spv_slice = unsafe { slice::from_raw_parts(spv, spv_count) };

    match _convert_spirv_to_wgsl(spv_slice) {
        Ok(wgsl) => {
            let wgsl_length = wgsl.chars().count();
            let wgsl_string = CString::new(wgsl).unwrap().into_raw();
            ConvertResult {
                wgsl_string,
                wgsl_length,
                error_string: ptr::null_mut(),
                error_length: 0,
            }
        }
        Err(error) => {
            let error_length = error.chars().count();
            let error_string = CString::new(error).unwrap().into_raw();
            ConvertResult {
                wgsl_string: ptr::null_mut(),
                wgsl_length: 0,
                error_string,
                error_length,
            }
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn convert_result_free(result: ConvertResult) {
    if !result.wgsl_string.is_null() {
        let _ = unsafe { CString::from_raw(result.wgsl_string) };
    }

    if !result.error_string.is_null() {
        let _ = unsafe { CString::from_raw(result.error_string) };
    }
}

// We ka-boom on failure.
fn _convert_spirv_to_wgsl(spv: &[u8]) -> Result<String, String> {
    let module = spv::parse_u8_slice(spv, &spv::Options::default()).unwrap();
    let caps = Capabilities::default()
        | Capabilities::PUSH_CONSTANT
        | Capabilities::STORAGE_TEXTURE_16BIT_NORM_FORMATS
        | Capabilities::SAMPLED_TEXTURE_AND_STORAGE_BUFFER_ARRAY_NON_UNIFORM_INDEXING;
    let mut validator = Validator::new(ValidationFlags::default(), caps);
    let info = validator.validate(&module).unwrap();

    Ok(wgsl::write_string(&module, &info, wgsl::WriterFlags::empty()).unwrap())
    // let module = spv::parse_u8_slice(spv, &spv::Options::default()).map_err(|e| e.to_string())?;
    // let mut validator = Validator::new(ValidationFlags::default(), Capabilities::default());
    // let info = validator.validate(&module).map_err(|e| e.to_string())?;

    // wgsl::write_string(&module, &info, wgsl::WriterFlags::empty()).map_err(|e| e.to_string())
}

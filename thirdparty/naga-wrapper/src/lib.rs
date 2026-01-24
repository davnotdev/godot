#![allow(clippy::missing_safety_doc)]

use naga::{
    back, front,
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
    spv_count: u32,
) -> ConvertResult {
    let spv_slice = unsafe { slice::from_raw_parts(spv, spv_count as usize) };

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

fn _convert_spirv_to_wgsl(spv: &[u8]) -> Result<String, String> {
    let caps = Capabilities::default()
        | Capabilities::IMMEDIATES
        | Capabilities::STORAGE_TEXTURE_16BIT_NORM_FORMATS
        | Capabilities::SHADER_FLOAT16_IN_FLOAT32
        | Capabilities::TEXTURE_AND_SAMPLER_BINDING_ARRAY
        | Capabilities::TEXTURE_AND_SAMPLER_BINDING_ARRAY_NON_UNIFORM_INDEXING
        | Capabilities::STORAGE_TEXTURE_BINDING_ARRAY
        | Capabilities::STORAGE_TEXTURE_BINDING_ARRAY_NON_UNIFORM_INDEXING
        | Capabilities::SUBGROUP;

    let module = front::spv::parse_u8_slice(spv, &front::spv::Options::default()).unwrap();
    let mut validator = Validator::new(ValidationFlags::default(), caps);
    let info = validator.validate(&module).unwrap();
    Ok(back::wgsl::write_string(&module, &info, back::wgsl::WriterFlags::empty()).unwrap())
}

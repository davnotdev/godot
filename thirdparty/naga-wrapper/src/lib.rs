#![allow(clippy::missing_safety_doc)]

use naga::{
    back::{self, PipelineConstants, pipeline_constants},
    front,
    valid::{Capabilities, ValidationFlags, Validator},
};
use std::{
    ffi::{CStr, CString, c_char},
    ptr, slice,
};

#[repr(C)]
pub struct ConvertResult {
    wgsl_string: *mut c_char,
    wgsl_length: usize,
    error_string: *mut c_char,
    error_length: usize,
}

#[repr(C)]
pub struct PipelineOverride {
    key: *const c_char,
    value: f64,
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn convert_spirv_to_wgsl_alloc(
    spv: *const u8,
    spv_count: u32,
    overrides: *const PipelineOverride,
    override_count: u32,
) -> ConvertResult {
    let spv_slice = unsafe { slice::from_raw_parts(spv, spv_count as usize) };
    let overrides = (override_count != 0).then(|| {
        unsafe { slice::from_raw_parts(overrides, override_count as usize) }
            .iter()
            .map(|p| {
                let c_str = unsafe { CStr::from_ptr(p.key) };
                let key = c_str.to_str().unwrap().to_owned();
                (key, p.value)
            })
            .collect::<PipelineConstants>()
    });

    match _convert_spirv_to_wgsl(spv_slice, overrides.as_ref()) {
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

fn _convert_spirv_to_wgsl(
    spv: &[u8],
    overrides: Option<&PipelineConstants>,
) -> Result<String, String> {
    let caps = Capabilities::default()
        | Capabilities::PUSH_CONSTANT
        | Capabilities::STORAGE_TEXTURE_16BIT_NORM_FORMATS
        | Capabilities::SHADER_FLOAT16_IN_FLOAT32
        | Capabilities::SAMPLED_TEXTURE_AND_STORAGE_BUFFER_ARRAY_NON_UNIFORM_INDEXING;

    let module = front::spv::parse_u8_slice(spv, &front::spv::Options::default()).unwrap();
    let mut validator = Validator::new(ValidationFlags::default(), caps);
    let info = validator.validate(&module).unwrap();

    let (module, info) = if let Some(overrides) = overrides {
        let (module, info) =
            pipeline_constants::process_overrides(&module, &info, None, overrides).unwrap();
        (module.into_owned(), info.into_owned())
    } else {
        (module, info)
    };

    Ok(back::wgsl::write_string(&module, &info, back::wgsl::WriterFlags::empty()).unwrap())
}

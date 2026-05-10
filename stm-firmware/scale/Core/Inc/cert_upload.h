/**
 ******************************************************************************
 * @file    cert_upload.h
 * @brief   Certificate upload functions for cellular module
 ******************************************************************************
 */

#ifndef __CERT_UPLOAD_H__
#define __CERT_UPLOAD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Upload a certificate to the cellular module
 * @param cert_data Pointer to certificate data (null-terminated string)
 * @param cert_name Name to give the certificate on the module
 * @param timeout_ms Timeout in milliseconds for the operation
 * @return true if upload successful, false otherwise
 */
bool CertUpload_Upload(const char *cert_data, const char *cert_name, uint32_t timeout_ms);

/**
 * @brief Upload the ISRG Root X1 certificate (Let's Encrypt)
 * @return true if upload successful, false otherwise
 */
bool CertUpload_UploadISRGRootX1(void);

#ifdef __cplusplus
}
#endif

#endif /* __CERT_UPLOAD_H__ */

/*****************************************************************************

Copyright (c) 1995, 2016, Oracle and/or its affiliates. All Rights Reserved.
Copyright (c) 2019, 2022, MariaDB Corporation.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA

*****************************************************************************/

/******************************************************************//**
@file include/fut0fut.h
File-based utilities

Created 12/13/1995 Heikki Tuuri
***********************************************************************/


#pragma once
#include "mtr0mtr.h"

MY_ATTRIBUTE((nonnull, warn_unused_result))
/** Gets a pointer to a file address and latches the page.
@param[in]	space		space id
@param[in]	zip_size	ROW_FORMAT=COMPRESSED page size, or 0
@param[in]	addr		file address
@param[in]	rw_latch	RW_S_LATCH, RW_X_LATCH, RW_SX_LATCH
@param[in,out]	mtr		mini-transaction
@param[out]	ptr_block	pointer to the block
@param[out]	err		error code, in case !*ptr_block
@return pointer to a byte in (*ptr_block)->frame; the *ptr_block is
bufferfixed and latched */
inline byte *fut_get_ptr(ulint space, ulint zip_size, fil_addr_t addr,
                         rw_lock_type_t rw_latch, mtr_t *mtr,
                         buf_block_t **ptr_block, dberr_t *err)
{
  ut_ad(addr.boffset < srv_page_size);
  ut_ad(rw_latch == RW_S_LATCH || rw_latch == RW_X_LATCH ||
        rw_latch == RW_SX_LATCH);
  buf_block_t *block= buf_page_get_gen(page_id_t{space, addr.page}, zip_size,
                                       rw_latch, nullptr,
                                       BUF_GET_POSSIBLY_FREED, mtr, err);
  if (block && block->page.is_freed())
    block= nullptr;

  *ptr_block= block;

  return block ? block->page.frame + addr.boffset : nullptr;
}

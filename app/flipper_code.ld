/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

. = ALIGN(0x1000);
*(SORT_BY_NAME(SORT_BY_ALIGNMENT(._flpr_code*)));

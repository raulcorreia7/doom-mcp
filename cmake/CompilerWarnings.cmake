function(dmcp_enable_warnings target)
    if(NOT MSVC)
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wunused
        )
    endif()
endfunction()

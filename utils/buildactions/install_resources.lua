require 'utils'

premake.modules.install_resources = {}

-- Config variables
local RESOURCES_URL = "https://github.com/multitheftauto/mtasa-resources/archive/refs/tags/1.5.9.zip"
local EXTRACT_DIR = "Bin/server/mods/deathmatch/resources/"

newaction {
    trigger = "install_resources",
    description = "Installs the resources to the bin directory",
    
    execute = function()
        -- Create a temporary directory for extraction
        local TEMP_DIR = "temp_resources_extract"
        if not os.mkdir(TEMP_DIR) then
            errormsg("ERROR: Couldn't create temporary directory")
            os.exit(1)
            return
        end
        
        -- Download resources
        if not http.download_print_errors(RESOURCES_URL, "temp_resources.zip") then
            os.exit(1)
            return
        end
        
        -- Extract resources to temporary directory
        if not os.extract_archive("temp_resources.zip", TEMP_DIR, true) then
            errormsg("ERROR: Couldn't unzip resources")
            os.exit(1)
            return
        end
        
        -- Move contents from the subfolder to the target directory
        local RESOURCES_SUBDIR = path.join(TEMP_DIR, "mtasa-resources-1.5.9")
        if not os.copydir(RESOURCES_SUBDIR, EXTRACT_DIR) then
            errormsg("ERROR: Couldn't move resources to target directory")
            os.exit(1)
            return
        end
        
        -- Cleanup
        if not os.remove("temp_resources.zip") then
            errormsg("ERROR: Couldn't delete downloaded resources zip file")
            -- Don't exit here as this is non-critical
        end
        
        if not os.rmdir(TEMP_DIR) then
            errormsg("WARNING: Couldn't remove temporary directory")
            -- Don't exit here as this is non-critical
        end
    end
}

return premake.modules.install_resources
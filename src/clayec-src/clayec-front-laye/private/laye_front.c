#include "kos/kos.h"
#include "kos/ansi.h"

#include "layec/front/laye/front.h"

#include "ast.h"
#include "token.h"
#include "parser.h"

typedef struct laye_parse_data
{
    layec_fileid fileId;
    laye_ast ast;
    arena_allocator* astArena;
} laye_parse_data;

static bool is_file_already_parsed(list(laye_parse_data) parseData, layec_fileid fileId)
{
    for (usize i = 0, iLen = arrlenu(parseData); i < iLen; i++)
    {
        if (parseData[i].fileId == fileId)
            return true;
    }

    return false;
}

static laye_parse_result_status parse_file(layec_context* context, list(laye_parse_data)* parseData, layec_fileid fileId)
{
    laye_parse_result parseResult = laye_parse(context, fileId);
    if (parseResult.status != LAYE_PARSE_OK)
        return parseResult.status;
    
    laye_parse_data d = {
        .fileId = fileId,
        .ast = parseResult.ast,
        .astArena = parseResult.astArena,
    };

    arrput(*parseData, d);
    
    string_view thisFileFullName = layec_context_get_file_full_path(context, fileId);
    for (usize j = 0, jLen = arrlenu(d.ast.imports); j < jLen; j++)
    {
        laye_ast_import import = d.ast.imports[j];
        // TODO(local): resolve this against various import locations and also lookup package names
        string_view name = string_slice(import.name, 0, import.name.count);
        layec_fileid importFileId = layec_context_add_file(context, name, thisFileFullName);
        if (importFileId == 0)
        {
            layec_issue_diagnostic(context, SEV_ERROR, import.location, "Unable to resolve import name '"STRING_VIEW_FORMAT"'", STRING_VIEW_EXPAND(name));
            continue;
        }

        if (!is_file_already_parsed(*parseData, importFileId))
        {
            laye_parse_result_status importParseStatus = parse_file(context, parseData, importFileId);
            if (importParseStatus != LAYE_PARSE_OK)
                return importParseStatus;
        }
    }

    return LAYE_PARSE_OK;
}

layec_front_end_status laye_front_end_entry(layec_context* context, list(layec_fileid) inputFiles)
{
    assert(context != nullptr);
    
    if (arrlenu(inputFiles) == 0)
        return LAYEC_FRONT_NO_INPUT_FILES;

    list(laye_parse_data) parseData = nullptr;
    for (usize i = 0; i < arrlenu(inputFiles); i++)
    {
        laye_parse_result_status parseStatus = parse_file(context, &parseData, inputFiles[i]);
        if (parseStatus != LAYE_PARSE_OK)
            return LAYEC_FRONT_PARSE_FAILED;
    }

    for (usize i = 0; i < arrlenu(parseData); i++)
    {
        laye_parse_data d = parseData[i];
        laye_ast_fprint(stderr, context, &d.ast, true);
    }

    // once we're done generating IR, destroy the AST memory arenas
    for (usize i = 0; i < arrlenu(parseData); i++)
        arena_destroy(parseData[i].astArena);

    return LAYEC_FRONT_SUCCESS;
}

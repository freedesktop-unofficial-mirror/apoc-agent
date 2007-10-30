/*
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS HEADER.
 * 
 * Copyright 2007 Sun Microsystems, Inc. All rights reserved.
 * 
 * The contents of this file are subject to the terms of either
 * the GNU General Public License Version 2 only ("GPL") or
 * the Common Development and Distribution License("CDDL")
 * (collectively, the "License"). You may not use this file
 * except in compliance with the License. You can obtain a copy
 * of the License at www.sun.com/CDDL or at COPYRIGHT. See the
 * License for the specific language governing permissions and
 * limitations under the License. When distributing the software,
 * include this License Header Notice in each file and include
 * the License file at /legal/license.txt. If applicable, add the
 * following below the License Header, with the fields enclosed
 * by brackets [] replaced by your own identifying information:
 * "Portions Copyrighted [year] [name of copyright owner]"
 * 
 * Contributor(s):
 * 
 * If you wish your version of this file to be governed by
 * only the CDDL or only the GPL Version 2, indicate your
 * decision by adding "[Contributor] elects to include this
 * software in this distribution under the [CDDL or GPL
 * Version 2] license." If you don't indicate a single choice
 * of license, a recipient has the option to distribute your
 * version of this file under either the CDDL, the GPL Version
 * 2 or to extend the choice of license to its licensees as
 * provided above. However, if you add GPL Version 2 code and
 * therefore, elected the GPL Version 2 license, then the
 * option applies only if the new code is made subject to such
 * option by the copyright holder.
 */

#include <papi/papi.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

char const * g_appname;
const char * g_entity = NULL;

void show_version(FILE * target);

void show_error(char const * fmt, char const * param);
void usage(char const * fmt, char const * param);
void list_usage();
void show_usage();
void offline_usage();

void test_connect_avail(int argc, char * const argv[]);
void list_components(int argc, char * const argv[]);
void show_components(int argc, char * const argv[]);
void do_offline(int argc, char * const argv[]);

int match(const char * input, const char * command)
{
    if (!input || !*input) return 0;
    
    return !strncmp(input,command,strlen(input));
}

int main(int argc, char * argv[])
{
    int argindex = 1;
    int subargc;
    char * const * subargv;
    
    if (argv[0] == NULL)
        g_appname = "[papitool]";
    else if ( (g_appname = strrchr(argv[0],'/')) != NULL )
        ++g_appname;
    else
        g_appname = argv[0];
    
    if (argc <= argindex) usage("Too few arguments: %s", "Missing Command");

    /* parse options */
    if (!strcmp(argv[argindex], "--version") ||
        !strcmp(argv[argindex], "-V") )
    {
        show_version(stdout);
        if (argc == ++argindex) exit(0);
    }

    if ( !strcmp(argv[argindex], "-e") || 
        (!strcmp(argv[argindex], "-Ve") && (show_version(stdout),1)) )
    {
        ++argindex;
        if (argc <= argindex) usage("Too few arguments: %s", "Missing Entity and Command");

        g_entity = argv[argindex++];

        if (argc <= argindex) usage("Too few arguments: %s", "Missing Command");
    }
    
    /* do help command/options */
    if (!strcmp(argv[argindex], "help") ||
        !strcmp(argv[argindex], "--help") ||
        !strcmp(argv[argindex], "-h") ||
        !strcmp(argv[argindex], "-?") )
    {
        usage(NULL,NULL);
    }

    /* end of options */
    subargc = argc - argindex;
    subargv = argv + argindex;
    
    if (match(subargv[0],"test"))
        test_connect_avail(subargc,subargv);
    
    else if (match(subargv[0],"list"))
        list_components(subargc,subargv);
    
    else if (match(subargv[0],"show"))
        show_components(subargc,subargv);
    
    else if (match(subargv[0],"offline"))
        do_offline(subargc,subargv);
    
    else
        usage("Unknown command: '%s'",subargv[0]);

    return 0;
}

/* PAPI helper functions */

void checkPapiError(PAPIStatus error)
{
    const char * errmsg;
    switch (error)
    {
    case PAPISuccess:
        return;
        
    case PAPIConnectionFailure:
        errmsg = "PAPI: Connection Failure for entity %s";
        break;

    case PAPIAuthenticationFailure:
        errmsg = "PAPI: Authentication Failure for entity %s";
        break;


    case PAPINoSuchComponentFailure:
        errmsg = "PAPI: No such component";
        break;

    case PAPILDBFailure:
        errmsg = "PAPI: Database (LDB) Failure";
        break;

    case PAPIInvalidArg:
        errmsg = "PAPI: Invalid Argument";
        break;

    case PAPIOutOfMemoryFailure:
        errmsg = "PAPI: Out of Memory";
        break;

    case PAPIUnknownFailure:
        errmsg = "PAPI: Unspecified Failure";
        break;

    default:
        errmsg = "PAPI: Unknown PAPI error code";
        break;
    }
    const char * entity = g_entity ? g_entity : "<default entity>";

    show_error(errmsg,entity);
    fprintf(stderr,"\nTerminating\n");

    exit(-error);
}

PAPI * initPapi()
{
    PAPIStatus error = PAPISuccess;
    PAPI * result = papiInitialise(g_entity,&error);
    
    checkPapiError(error);
    return result;
}

void exitPapi(PAPI * papi)
{
    PAPIStatus error = PAPISuccess;

    papiDeinitialise(papi,&error);
    
    checkPapiError(error);
}

/* connection test function */

void describe_test_usage()
{
    fprintf(stderr,"\n\n\t%s [options] t[est]", g_appname);
    fputs(  "\n\t\t- Test if a PAPI connection can be established"
            , stderr);
}

void test_connect_avail(int argc, char * const argv[])
{
    PAPI * papi;
    
    papi = initPapi();

    fprintf(stderr,"\nOK. PAPI connection was established successfully");
    if (g_entity)
        fprintf(stderr," for entity '%s'", g_entity);
    else
        fprintf(stderr," for the current user");
    fputs("\n",stderr);

    exitPapi(papi);
}

/* list components function */

void describe_list_usage()
{
    fprintf(stderr,"\n\n\t%s [options] l[ist] [filter]", g_appname);
    fputs(  "\n\t\t- List components that match a filter"
            , stderr);
}

void list_components(int argc, char * const argv[])
{
    PAPI * papi;
    PAPIStatus error = PAPISuccess;
    PAPIStringList * components;
    PAPIStringList * component;
    const char * filter;
    int count = 0;

    if (argc > 2)
    {
        show_error("Too many arguments for 'list' command",NULL);
        list_usage();
    }

    if (argc < 2)
        filter = NULL;
    else
        filter = argv[1];

    papi = initPapi();

    components = papiListComponentNames(papi,filter,&error);
        
    if (error == PAPINoSuchComponentFailure)
    {
        fprintf(stderr,"\nNo Component for filter '%s'\n",filter ? filter : "<null>");
        goto done;
    }
    
    checkPapiError(error);

    for (component = components; component; component=component->next)
    {
        ++count;
        if (component->data)
            printf("%s\n",component->data);
        else
            fprintf(stderr, "### Data for component #%d is NULL ###\n",count);
    }
    fprintf(stderr,"\n\n%d component(s) found.\n",count );
    
    papiFreeStringList(components);
                                  
done:
    exitPapi(papi);
}

/* show component data function */

void describe_show_usage()
{
    fprintf(stderr,"\n\n\t%s [options] s[how] (all|time|count|<N>) <component> ...", g_appname);

    fputs(  "\n\t\t- Show data for component(s)."
            "\n\t\t  Supported operations:"
            "\n\t\t     all   - show entire contents for all layers of each component"
            "\n\t\t     time  - show layer count and timestamp for each component"
            "\n\t\t     count - show layer count for each component"
            "\n\t\t      <N>  - show Nth layer of component (works best with a single component)"
            , stderr);
}

enum { 
    SHOW_FULL  = -1,
    SHOW_TIME  = -2,
    SHOW_COUNT = -3
};

void dump_layer(PAPILayerList * layer, FILE * tofile)
{
    fputs("\n",tofile);
    if (layer && layer->data && layer->size>0)
    {
        size_t size = layer->size, read;
        const unsigned char * data = layer->data;

        while (size > 0)
        {
            read = fwrite(data,1,size,tofile);

            if (read == 0)
            {
                fputs("\n### ERROR ###",tofile);
                show_error("Error writing output: %s", strerror(errno));
                break;
            }

            size -= read;
            data += read;
        }
    }
    else
        fputs("### No data for this layer ###",tofile);
    
    fputs("\n",tofile);
}

void dump_component(const char * component, PAPILayerList * layers, int op)
{
    PAPILayerList * layer;
    int count = 0;

    if (op >= SHOW_FULL)
        printf("\n========== Start of data for component '%s' ==================", component);
    else 
        printf("\nComponent '%s':", component);
    
    for (layer = layers; layer; layer=layer->next)
    {
        ++count;

        if (op == SHOW_FULL || op == SHOW_TIME)
            printf("\n- Layer #%d: Timestamp = %s\t***",
                    count, layer->timestamp ? layer->timestamp : "<null>");
        
        if (op == SHOW_FULL || op == count)
            dump_layer(layer,stdout);
    }
       
    if (op >= SHOW_FULL)
        printf("========== End of data for component '%s' ====================", component);

    printf("\n\t%d layer(s)\n",count);
}

void show_components(int argc, char * const argv[])
{
    PAPI * papi;
    PAPIStatus error = PAPISuccess;
    const char * component;
    int argindex = 1;
    int op;

    if (argc < 3)
    {
        show_error("Not enough arguments for 'show' command",NULL);
        show_usage();
    }

    if (match(argv[argindex],"all"))
        op = SHOW_FULL;
    
    else if (match(argv[argindex],"time"))
        op = SHOW_TIME;

    else if (match(argv[argindex],"count"))
        op = SHOW_COUNT;
    
    else
        op = atoi(argv[argindex]);
    
    if (op == 0)
    {
        show_error("Invalid operation '%s' for 'show' command",argv[argindex]);
        show_usage();
    }

    papi = initPapi();

    while ( (component = argv[++argindex]) != NULL )
    {
        PAPILayerList * layers = papiReadComponentLayers(papi,component,&error);
        
        if (error == PAPINoSuchComponentFailure)
        {
            printf("\nNo Such Component '%s'\n",component);
            continue;
        }
        
        checkPapiError(error);

        dump_component(component,layers,op);

        papiFreeLayerList(layers);
    }                             
                                  
    exitPapi(papi);               
}                                 

/* offline status function */

void describe_offline_usage()
{
    fprintf(stderr,"\n\n\t%s [options] o[ffline] (show|add|drop|+|-) <component> ...", g_appname);

    fputs(  "\n\t\t- Show or change offline status for component(s)."
            "\n\t\t  Supported operations:"
            "\n\t\t     show  - show current offline status for each component"
            "\n\t\t     add   - mark each component for offline availability  (alias '+')"
            "\n\t\t     drop  - mark each component as not availabile offline (alias '-')"
            , stderr);
    fputs("\n\t*** NOT YET IMPLEMENTED ***",stderr);
}

void do_offline(int argc, char * const argv[])
{
    PAPI * papi;
    PAPIStatus error = PAPISuccess;

    offline_usage();
    
    papi = initPapi();
    exitPapi(papi);
}

/* usage reporting */

void show_version(FILE * file)
{
    const char * version = "0.1";
    const char * author = "Joerg Barfurth";
    fprintf(file,"\n%s - PAPI dumper tool for APOC",g_appname);
    fprintf(file,"\nVersion: %s\tBuild time: %s",version,__TIME__);
    fprintf(file,"\nAuthor:  %s",author);
    fprintf(file,"\nCopyright (C) 2003 Sun Microsystems,Inc.\n");
}

void pre_usage()
{
    show_version(stderr);
    fprintf(stderr,"\nUsage: ");
}

void post_usage(int exitcode)
{
    fprintf(stderr,"\n\nOptions:");
    fprintf(stderr,"\n\t-V / --version\t- display information about this tool");
    fprintf(stderr,"\n\t-e <entity>\t- use entity id <entity>");
    fprintf(stderr,"\n");

    exit(exitcode);
}

void describe_help_usage()
{
    fprintf(stderr,"\n\n\t%s [options] help\n\t\t- show this help screen", g_appname);
}

void show_error(char const * fmt, char const * param)
{
    fprintf(stderr,"### %s Error: ", g_appname);
    fprintf(stderr,fmt,param);
}

void usage(char const * fmt, char const * param)
{
    if (fmt)
        show_error(fmt,param);
    
    pre_usage();

    describe_test_usage();
    describe_list_usage();
    describe_show_usage();
    describe_offline_usage();
    describe_help_usage();

    post_usage(fmt?1:0);
}

void list_usage()
{
    pre_usage();

    describe_list_usage();

    post_usage(2);
}

void show_usage()
{
    pre_usage();

    describe_show_usage();

    post_usage(3);
}

void offline_usage()
{
    pre_usage();

    describe_offline_usage();

    post_usage(4);
}



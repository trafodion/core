/*
 *(C) Copyright 2015 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.trafodion.rest;

import java.io.*;
import java.util.*;

import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.Request;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.UriInfo;
import javax.ws.rs.core.MultivaluedMap;
import javax.ws.rs.core.Response.ResponseBuilder;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;

import org.codehaus.jettison.json.JSONArray;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;

import org.trafodion.rest.script.ScriptManager;
import org.trafodion.rest.script.ScriptContext;

import org.trafodion.rest.Constants;
import org.trafodion.rest.util.Bytes;
import org.trafodion.rest.zookeeper.ZkClient;
import org.trafodion.rest.RestConstants;

public class TransactionsResource extends ResourceBase {
	private static final Log LOG =
		LogFactory.getLog(TransactionsResource.class);
	private String operation = "all";

	static CacheControl cacheControl;
	static {
		cacheControl = new CacheControl();
		cacheControl.setNoCache(true);
		cacheControl.setNoTransform(false);
	}

	/**
	 * Constructor
	 * @throws IOException
	 */
	public TransactionsResource(String type) throws IOException {
		super();
		this.operation = type;
	}
	
	public String stats() throws IOException {
	    ScriptContext scriptContext = new ScriptContext();
	    scriptContext.setScriptName(Constants.SYS_SHELL_SCRIPT_NAME);
	    scriptContext.setCommand("dtmci stats -j");

	    try {
	        ScriptManager.getInstance().runScript(scriptContext);//This will block while script is running
	    } catch (Exception e) {
	        e.printStackTrace();
	        throw new IOException(e);
	    }

        if(LOG.isDebugEnabled()) {
            StringBuilder sb = new StringBuilder();
            sb.append("exit code [" + scriptContext.getExitCode() + "]");
            if(! scriptContext.getStdOut().toString().isEmpty()) 
                sb.append(", stdout [" + scriptContext.getStdOut().toString() + "]");
            if(! scriptContext.getStdErr().toString().isEmpty())
                sb.append(", stderr [" + scriptContext.getStdErr().toString() + "]");
            LOG.debug(sb.toString());
        }

        return scriptContext.getStdOut().toString();
	}

	public String tm() throws IOException {
	    ScriptContext scriptContext = new ScriptContext();
	    scriptContext.setScriptName(Constants.SYS_SHELL_SCRIPT_NAME);
	    scriptContext.setCommand("dtmci status tm -j");

	    try {
	        ScriptManager.getInstance().runScript(scriptContext);//This will block while script is running
	    } catch (Exception e) {
	        e.printStackTrace();
	        throw new IOException(e);
	    }

	    if(LOG.isDebugEnabled()) {
	        StringBuilder sb = new StringBuilder();
	        sb.append("exit code [" + scriptContext.getExitCode() + "]");
	        if(! scriptContext.getStdOut().toString().isEmpty()) 
	            sb.append(", stdout [" + scriptContext.getStdOut().toString() + "]");
	        if(! scriptContext.getStdErr().toString().isEmpty())
	            sb.append(", stderr [" + scriptContext.getStdErr().toString() + "]");
	        LOG.debug(sb.toString());
	    }

	    return scriptContext.getStdOut().toString();
	}

	@GET
	@Produces({MIMETYPE_JSON})
	public Response get(final @Context UriInfo uriInfo,@Context Request request) {
	    try {
	        if (LOG.isDebugEnabled()) {
	            LOG.debug("GET " + uriInfo.getAbsolutePath());

	            MultivaluedMap<String, String> queryParams = uriInfo.getQueryParameters();
	            String output = " Query Parameters :\n";
	            for (String key : queryParams.keySet()) {
	                output += key + " : " + queryParams.getFirst(key) +"\n";
	            }
	            LOG.debug(output);

	            MultivaluedMap<String, String> pathParams = uriInfo.getPathParameters();
	            output = " Path Parameters :\n";
	            for (String key : pathParams.keySet()) {
	                output += key + " : " + pathParams.getFirst(key) +"\n";
	            }
	            LOG.debug(output);
	        }

	        String result = null;
	        if(operation.equals("stats"))
	            result = stats();
	        else
	            result = tm();

	        ResponseBuilder response = Response.ok(result);
	        response.cacheControl(cacheControl);
	        return response.build();
	    } catch (IOException e) {
	        e.printStackTrace();
	        return Response.status(Response.Status.SERVICE_UNAVAILABLE)
	                .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
	                .build();
	    }
	}
}

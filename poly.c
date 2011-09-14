typedef struct {
    void            *pdest;
    short           *pz;
    int             count;
    byte            *ptex;
    int             sfrac, tfrac, light, zi;
} spanpackage_t;

void D_PolysetDraw(void)
/*
Definition at line 132 of file d_polyse.c.

References CACHE_SIZE, D_DrawNonSubdiv(), D_DrawSubdiv(), DPS_MAXSPANS, affinetridesc_t::drawtype, and r_affinetridesc.

Referenced by R_AliasClipTriangle(), R_AliasPreparePoints(), and R_AliasPrepareUnclippedPoints().
*/
{
    spanpackage_t   spans[DPS_MAXSPANS + 1 +
            ((CACHE_SIZE - 1) / sizeof(spanpackage_t)) + 1];
                        // one extra because of cache line pretouching

    a_spans = (spanpackage_t *)
            (((long)&spans[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));

    if (r_affinetridesc.drawtype)
    {
        D_DrawSubdiv ();
    }
    else
    {
        D_DrawNonSubdiv ();
    }
}

typedef struct mtriangle_s {
    int                 facesfront;
    int                 vertindex[3];
} mtriangle_t;

typedef struct finalvert_s {
    int     v[6];       // u, v, s, t, l, 1/z
    int     flags;
    float   reserved;
} finalvert_t;

void D_DrawSubdiv()
/*
Definition at line 190 of file d_polyse.c.

References acolormap, ALIAS_ONSEAM, d_pcolormap, D_PolysetRecursiveTriangle(), finalvert_s::flags, affinetridesc_t::numtriangles, affinetridesc_t::pfinalverts, affinetridesc_t::ptriangles, r_affinetridesc, affinetridesc_t::seamfixupX16, finalvert_s::v, and mtriangle_s::vertindex.

Referenced by D_PolysetDraw().
*/
{
    mtriangle_t     *ptri;
    finalvert_t     *pfv, *index0, *index1, *index2;
    int             i;
    int             lnumtriangles;

    pfv = r_affinetridesc.pfinalverts;
    ptri = r_affinetridesc.ptriangles;
    lnumtriangles = r_affinetridesc.numtriangles;

#ifdef FTE_PEXT_TRANS
#ifdef GLQUAKE
    if (transbackfac)
    {
        if (r_pixbytes == 4)
            drawfnc = D_PolysetRecursiveTriangle32Trans;
        else if (r_pixbytes == 2)
            drawfnc = D_PolysetRecursiveTriangle16C;
        else //if (r_pixbytes == 1)
            drawfnc = D_PolysetRecursiveTriangleTrans;
    }
    else
#endif
#endif

    for (i=0 ; i<lnumtriangles ; i++)
    {
        index0 = pfv + ptri[i].vertindex[0];
        index1 = pfv + ptri[i].vertindex[1];
        index2 = pfv + ptri[i].vertindex[2];

        if (((index0->v[1]-index1->v[1]) *
             (index0->v[0]-index2->v[0]) -
             (index0->v[0]-index1->v[0]) * 
             (index0->v[1]-index2->v[1])) >= 0)
        {
            continue;
        }

        d_pcolormap = &((byte *)acolormap)[index0->v[4] & 0xFF00];

        if (ptri[i].facesfront)
        {
            D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);
        }
        else
        {
            int     s0, s1, s2;

            s0 = index0->v[2];
            s1 = index1->v[2];
            s2 = index2->v[2];

            if (index0->flags & ALIAS_ONSEAM)
                index0->v[2] += r_affinetridesc.seamfixupX16;
            if (index1->flags & ALIAS_ONSEAM)
                index1->v[2] += r_affinetridesc.seamfixupX16;
            if (index2->flags & ALIAS_ONSEAM)
                index2->v[2] += r_affinetridesc.seamfixupX16;

            D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);

            index0->v[2] = s0;
            index1->v[2] = s1;
            index2->v[2] = s2;
        }
    }
}

void DrawNonSubdiv(void)
/*
Definition at line 266 of file d_polyse.c.

References ALIAS_ONSEAM, D_PolysetSetEdgeTable(), D_RasterizeAliasPolySmooth(), d_xdenom, mtriangle_s::facesfront, finalvert_s::flags, affinetridesc_t::numtriangles, affinetridesc_t::pfinalverts, affinetridesc_t::ptriangles, r_affinetridesc, r_p0, r_p1, r_p2, affinetridesc_t::seamfixupX16, finalvert_s::v, and mtriangle_s::vertindex.

Referenced by D_PolysetDraw().
*/
{
    mtriangle_t     *ptri;
    finalvert_t     *pfv, *index0, *index1, *index2;
    int             i;
    int             lnumtriangles;

    pfv = r_affinetridesc.pfinalverts;
    ptri = r_affinetridesc.ptriangles;
    lnumtriangles = r_affinetridesc.numtriangles;

    for (i=0 ; i<lnumtriangles ; i++, ptri++)
    {
        index0 = pfv + ptri->vertindex[0];
        index1 = pfv + ptri->vertindex[1];
        index2 = pfv + ptri->vertindex[2];

        d_xdenom = (index0->v[1]-index1->v[1]) *
                (index0->v[0]-index2->v[0]) -
                (index0->v[0]-index1->v[0])*(index0->v[1]-index2->v[1]);

        if (d_xdenom >= 0)
        {
            continue;
        }

        r_p0[0] = index0->v[0];     // u
        r_p0[1] = index0->v[1];     // v
        r_p0[2] = index0->v[2];     // s
        r_p0[3] = index0->v[3];     // t
        r_p0[4] = index0->v[4];     // light
        r_p0[5] = index0->v[5];     // iz

        r_p1[0] = index1->v[0];
        r_p1[1] = index1->v[1];
        r_p1[2] = index1->v[2];
        r_p1[3] = index1->v[3];
        r_p1[4] = index1->v[4];
        r_p1[5] = index1->v[5];

        r_p2[0] = index2->v[0];
        r_p2[1] = index2->v[1];
        r_p2[2] = index2->v[2];
        r_p2[3] = index2->v[3];
        r_p2[4] = index2->v[4];
        r_p2[5] = index2->v[5];

        if (!ptri->facesfront)
        {
            if (index0->flags & ALIAS_ONSEAM)
                r_p0[2] += r_affinetridesc.seamfixupX16;
            if (index1->flags & ALIAS_ONSEAM)
                r_p1[2] += r_affinetridesc.seamfixupX16;
            if (index2->flags & ALIAS_ONSEAM)
                r_p2[2] += r_affinetridesc.seamfixupX16;
        }

        D_PolysetSetEdgeTable ();
        D_RasterizeAliasPolySmooth ();
    }
}

void D_PolysetSetEdgeTable(void)
/*
Definition at line 956 of file d_polyse.c.

References r_p0, r_p1, and r_p2.

Referenced by D_DrawNonSubdiv().
*/
{
    int         edgetableindex;

    edgetableindex = 0; // assume the vertices are already in
                        //  top to bottom order

//
// determine which edges are right & left, and the order in which
// to rasterize them
//
    if (r_p0[1] >= r_p1[1])
    {
        if (r_p0[1] == r_p1[1])
        {
            if (r_p0[1] < r_p2[1])
                pedgetable = &edgetables[2];
            else
                pedgetable = &edgetables[5];

            return;
        }
        else
        {
            edgetableindex = 1;
        }
    }

    if (r_p0[1] == r_p2[1])
    {
        if (edgetableindex)
            pedgetable = &edgetables[8];
        else
            pedgetable = &edgetables[9];

        return;
    }
    else if (r_p1[1] == r_p2[1])
    {
        if (edgetableindex)
            pedgetable = &edgetables[10];
        else
            pedgetable = &edgetables[11];

        return;
    }

    if (r_p0[1] > r_p2[1])
        edgetableindex += 2;

    if (r_p1[1] > r_p2[1])
        edgetableindex += 4;

    pedgetable = &edgetables[edgetableindex];
}

void D_RasterizeAliasPolySmooth(void)
/*
Definition at line 742 of file d_polyse.c.

References a_spans, spanpackage_t::count, d_aspancount, d_countextrastep, d_light, d_lightbasestep, d_lightextrastep, d_pdest, d_pdestbasestep, d_pdestextrastep, D_PolysetCalcGradients(), D_PolysetDrawSpans8(), D_PolysetScanLeftEdge(), D_PolysetSetUpForLineScan(), d_ptex, d_ptexbasestep, d_ptexextrastep, d_pz, d_pzbasestep, d_pzbuffer, d_pzextrastep, d_sfrac, d_sfracbasestep, d_sfracextrastep, d_tfrac, d_tfracbasestep, d_tfracextrastep, d_viewbuffer, d_zi, d_zibasestep, d_ziextrastep, d_zwidth, edgetable::numleftedges, edgetable::numrightedges, edgetable::pleftedgevert0, edgetable::pleftedgevert1, edgetable::pleftedgevert2, edgetable::prightedgevert0, edgetable::prightedgevert1, edgetable::prightedgevert2, affinetridesc_t::pskin, r_affinetridesc, r_lstepx, r_lstepy, r_sstepx, r_sstepy, r_tstepx, r_tstepy, r_zistepx, r_zistepy, screenwidth, affinetridesc_t::skinwidth, ubasestep, and ystart.

Referenced by D_DrawNonSubdiv().
*/
{
    int             initialleftheight, initialrightheight;
    int             *plefttop, *prighttop, *pleftbottom, *prightbottom;
    int             working_lstepx, originalcount;

    plefttop = pedgetable->pleftedgevert0;
    prighttop = pedgetable->prightedgevert0;

    pleftbottom = pedgetable->pleftedgevert1;
    prightbottom = pedgetable->prightedgevert1;

    initialleftheight = pleftbottom[1] - plefttop[1];
    initialrightheight = prightbottom[1] - prighttop[1];

//
// set the s, t, and light gradients, which are consistent across the triangle
// because being a triangle, things are affine
//
    D_PolysetCalcGradients (r_affinetridesc.skinwidth);

//
// rasterize the polygon
//

//
// scan out the top (and possibly only) part of the left edge
//
    D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
                          pleftbottom[0], pleftbottom[1]);

    d_pedgespanpackage = a_spans;

    ystart = plefttop[1];
    d_aspancount = plefttop[0] - prighttop[0];

    d_ptex = (byte *)r_affinetridesc.pskin + (plefttop[2] >> 16) +
            (plefttop[3] >> 16) * r_affinetridesc.skinwidth;
#if id386
    d_sfrac = (plefttop[2] & 0xFFFF) << 16;
    d_tfrac = (plefttop[3] & 0xFFFF) << 16;
    d_pzbasestep = (d_zwidth + ubasestep) << 1;
    d_pzextrastep = d_pzbasestep + 2;
#else
    d_sfrac = plefttop[2] & 0xFFFF;
    d_tfrac = plefttop[3] & 0xFFFF;
    d_pzbasestep = d_zwidth + ubasestep;
    d_pzextrastep = d_pzbasestep + 1;
#endif
    d_light = plefttop[4];
    d_zi = plefttop[5];

    d_pdestbasestep = screenwidth + ubasestep;
    d_pdestextrastep = d_pdestbasestep + 1;
    d_pdest = (byte *)d_viewbuffer +
            ystart * screenwidth + plefttop[0];
    d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

// TODO: can reuse partial expressions here

// for negative steps in x along left edge, bias toward overflow rather than
// underflow (sort of turning the floor () we did in the gradient calcs into
// ceil (), but plus a little bit)
    if (ubasestep < 0)
        working_lstepx = r_lstepx - 1;
    else
        working_lstepx = r_lstepx;

    d_countextrastep = ubasestep + 1;
    d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
            ((r_tstepy + r_tstepx * ubasestep) >> 16) *
            r_affinetridesc.skinwidth;
#if id386
    d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) << 16;
    d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) << 16;
#else
    d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
    d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
#endif
    d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
    d_zibasestep = r_zistepy + r_zistepx * ubasestep;

    d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
            ((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
            r_affinetridesc.skinwidth;
#if id386
    d_sfracextrastep = (r_sstepy + r_sstepx*d_countextrastep) << 16;
    d_tfracextrastep = (r_tstepy + r_tstepx*d_countextrastep) << 16;
#else
    d_sfracextrastep = (r_sstepy + r_sstepx*d_countextrastep) & 0xFFFF;
    d_tfracextrastep = (r_tstepy + r_tstepx*d_countextrastep) & 0xFFFF;
#endif
    d_lightextrastep = d_lightbasestep + working_lstepx;
    d_ziextrastep = d_zibasestep + r_zistepx;

    D_PolysetScanLeftEdge (initialleftheight);

//
// scan out the bottom part of the left edge, if it exists
//
    if (pedgetable->numleftedges == 2)
    {
        int     height;

        plefttop = pleftbottom;
        pleftbottom = pedgetable->pleftedgevert2;

        D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
                              pleftbottom[0], pleftbottom[1]);

        height = pleftbottom[1] - plefttop[1];

// TODO: make this a function; modularize this function in general

        ystart = plefttop[1];
        d_aspancount = plefttop[0] - prighttop[0];
        d_ptex = (byte *)r_affinetridesc.pskin + (plefttop[2] >> 16) +
                (plefttop[3] >> 16) * r_affinetridesc.skinwidth;
        d_sfrac = 0;
        d_tfrac = 0;
        d_light = plefttop[4];
        d_zi = plefttop[5];

        d_pdestbasestep = screenwidth + ubasestep;
        d_pdestextrastep = d_pdestbasestep + 1;
        d_pdest = (byte *)d_viewbuffer + ystart * screenwidth + plefttop[0];
#if id386
        d_pzbasestep = (d_zwidth + ubasestep) << 1;
        d_pzextrastep = d_pzbasestep + 2;
#else
        d_pzbasestep = d_zwidth + ubasestep;
        d_pzextrastep = d_pzbasestep + 1;
#endif
        d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

        if (ubasestep < 0)
            working_lstepx = r_lstepx - 1;
        else
            working_lstepx = r_lstepx;

        d_countextrastep = ubasestep + 1;
        d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
                ((r_tstepy + r_tstepx * ubasestep) >> 16) *
                r_affinetridesc.skinwidth;
#if id386
        d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) << 16;
        d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) << 16;
#else
        d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
        d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
#endif
        d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
        d_zibasestep = r_zistepy + r_zistepx * ubasestep;

        d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
                ((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
                r_affinetridesc.skinwidth;
#if id386
        d_sfracextrastep = ((r_sstepy+r_sstepx*d_countextrastep) & 0xFFFF)<<16;
        d_tfracextrastep = ((r_tstepy+r_tstepx*d_countextrastep) & 0xFFFF)<<16;
#else
        d_sfracextrastep = (r_sstepy+r_sstepx*d_countextrastep) & 0xFFFF;
        d_tfracextrastep = (r_tstepy+r_tstepx*d_countextrastep) & 0xFFFF;
#endif
        d_lightextrastep = d_lightbasestep + working_lstepx;
        d_ziextrastep = d_zibasestep + r_zistepx;

        D_PolysetScanLeftEdge (height);
    }

// scan out the top (and possibly only) part of the right edge, updating the
// count field
    d_pedgespanpackage = a_spans;

    D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
                          prightbottom[0], prightbottom[1]);
    d_aspancount = 0;
    d_countextrastep = ubasestep + 1;
    originalcount = a_spans[initialrightheight].count;
    a_spans[initialrightheight].count = -999999; // mark end of the spanpackages
    D_PolysetDrawSpans8 (a_spans);

// scan out the bottom part of the right edge, if it exists
    if (pedgetable->numrightedges == 2)
    {
        int             height;
        spanpackage_t   *pstart;

        pstart = a_spans + initialrightheight;
        pstart->count = originalcount;

        d_aspancount = prightbottom[0] - prighttop[0];

        prighttop = prightbottom;
        prightbottom = pedgetable->prightedgevert2;

        height = prightbottom[1] - prighttop[1];

        D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
                              prightbottom[0], prightbottom[1]);

        d_countextrastep = ubasestep + 1;
        a_spans[initialrightheight + height].count = -999999;
                                            // mark end of the spanpackages
        D_PolysetDrawSpans8 (pstart);
    }
}


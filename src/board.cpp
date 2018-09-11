#include "include/board.h"

CBoard::CBoard() : miSize{2}
{
    // Intentionally left blank.
}

CBoard::CBoard(const CBoard& aCls) : miSize{aCls.miSize}
{
    if (!aCls.mpBoardCombs.empty())
    {
        mpBoardCombs = aCls.mpBoardCombs;
    }
}

CBoard::~CBoard()
{
    for (CombIterator pIter = mpBoardCombs.begin(); pIter != mpBoardCombs.end(); ++pIter)
    {
        if (nullptr != *pIter)
        {
            delete (*pIter);
        }
    }
}

CBoard& CBoard::operator =(const CBoard& aCls)
{
    if (this != &aCls)
    {
        miSize = aCls.miSize;
        mpBoardCombs = aCls.mpBoardCombs;
    }

    return *this;
}

/*!
 * \brief CBoard::Create
 *
 * This method is used to "create" a new board. This invloves heap-allocating (if needed) new honeycombs and tessellating them accordingly.
 *
 * \param uCellSz - This is the size of the cells for the combs.
 * \param aqCenter - The center of the board.
 */
void CBoard::Create(u32 uCellSz, QPointF aqCenter)
{
    const u32 c_uCellRadius = uCellSz / 2;
    const float c_nCombSize = static_cast<float>(c_uCellRadius * CELL_COMB_RATIO);
    float nTessSz = (c_nCombSize * TESS_COMBSZ_TO_TESSSZ);

    // Explicitly set the center comb.
    QPointF qStartPos = aqCenter;

    CHoneyComb* pTmpComb = new CHoneyComb();
    pTmpComb->SetCellSize(uCellSz);
    pTmpComb->SetPosition(qStartPos);
    mpBoardCombs.push_back(pTmpComb);

    if (miSize > 0)
    {
        // Set the starting position of the first layer.
        float nX = qStartPos.x() + (nTessSz * TESS_X_SHIFT);
        float nY = qStartPos.y() - (nTessSz * TESS_Y_SHIFT);

        qStartPos.setX(nX);
        qStartPos.setY(nY);

        u32 eClr = static_cast<u32>(Cell_Red);

        // Iterate over the layers, adding the appropriate number of combs.
        for (u32 iLyrIdx = 0; iLyrIdx < miSize; ++iLyrIdx)
        {
            // Add the new combs.
            std::vector<QPointF> vLayerPos = CalcTessPos(qStartPos, iLyrIdx, uCellSz);

            qInfo("Positioning %lu honeycombs...", vLayerPos.size());
            for (std::vector<QPointF>::iterator pPtIter = vLayerPos.begin(); pPtIter != vLayerPos.end(); ++pPtIter)
            {
                CHoneyComb* pTmpComb = new CHoneyComb();
                pTmpComb->SetCellSize(uCellSz);
                pTmpComb->SetPosition((*pPtIter));
                pTmpComb->SetCombColor(static_cast<ECellColors>(eClr));
                mpBoardCombs.push_back(pTmpComb);

                ++eClr;
                if (static_cast<u32>(Comb_Mixed) == eClr) { eClr = static_cast<u32>(Cell_Red); }
            }

            // Update the tessellation size.
            nTessSz = (nTessSz * TESS_NEAR_TO_FAR);

            // Update the starting position.
            nX = qStartPos.x() + (nTessSz * TESS_X_SHIFT);
            nY = qStartPos.y() + (nTessSz * TESS_Y_SHIFT);

            qStartPos.setX(nX);
            qStartPos.setY(nY);
        }
    }
}

/*!
 * \brief CBoard::Destroy
 *
 * This method is used to "destroy" an existing board. This involves going through and deleting the individual combs in the comb array, not the array itself!
 */
void CBoard::Destroy()
{
    for (CombIterator pIter = mpBoardCombs.begin(); pIter != mpBoardCombs.end(); ++pIter)
    {
        if (nullptr != *pIter)
        {
            delete (*pIter);
        }
    }
}

/*!
 * \brief CBoard::Draw
 *
 * This function simply iterates over the comb array and calls "Draw" on the combs.
 * \param pPainter - Pointer to a device context to paint to.
 */
void CBoard::Draw(QPainter *pPainter)
{
    if (nullptr != pPainter)
    {
        for (CombIterator pIter = mpBoardCombs.begin(); pIter != mpBoardCombs.end(); ++pIter)
        {
            if (nullptr != (*pIter))
            {
                (*pIter)->Draw(pPainter);
            }
        }
    }
}

/*!
 * \brief CBoard::GetBoardSize
 *
 * This method returns the number of tessellation rows set for the current/next board.
 *
 * \return unsigned int representing tessellation rows.
 */
u32 CBoard::GetBoardSize()
{
    return miSize;
}

/*!
 * \brief CBoard::GetCombIterator
 *
 * This function simply returns an iterator object to the board's honeycombs.
 *
 * \return CombIterator pointing to the first honeycomb.
 */
CombIterator CBoard::GetCombIterator()
{
    return mpBoardCombs.begin();
}

/*!
 * \brief CBoard::GetComb
 *
 * This function returns a pointer to the honeycomb referenced by the iterator.
 *
 * \param pIter - Valid honeycomb iterator for the board.
 * \return Honeycomb object being referenced, or nullptr if none exists.
 */
CHoneyComb* CBoard::GetComb(CombIterator pIter)
{
    return (*pIter);
}

/*!
 * \brief CBoard::GetComb [overloaded]
 *
 * This function returns a pointer to the honeycomb at the given index.
 *
 * \param uCombIdx - Index to the comb to return.
 * \return Honeycomb object being referenced, or nullptr if none exists.
 */
CHoneyComb* CBoard::GetComb(u32 uCombIdx)
{
    return (uCombIdx < mpBoardCombs.size()) ? mpBoardCombs[uCombIdx] : nullptr;
}

/*!
 * \brief CBoard::GetNeighbors
 *
 * This function will return all the neighbors for the given honeycomb.
 *
 * \note This function will heap-allocate data and it's up to the client to free this memory!
 *
 * \param pComb - Honeycomb to gather the neighbors of.
 * \return CHoneyComb pointer array of the neighbors, or nullptr if no neighbors.
 */
CHoneyComb** CBoard::GetNeighbors(CHoneyComb *pComb)
{
    CHoneyComb** pvNeighbors = nullptr; // Maximum of 6 neighbors.
    if (nullptr != pComb)
    {
        pvNeighbors = new CHoneyComb*[NUM_HEX_VERTS]; // Maximum of 6 neighbors.
        if (nullptr != pvNeighbors)
        {
            // Get this comb's position and size.
            const float c_nTessSz = (pComb->GetCombSize() * TESS_COMBSZ_TO_TESSSZ);
            const QPointF& c_qPos = pComb->GetPosition();

            // Now, iterate over the combs and collect all 6 neighbors (if possible).
            u32 iCombIdx = 0;
            for (CombIterator pIter = GetCombIterator(); pIter != mpBoardCombs.end(); ++pIter)
            {
                // Setup our collision variables.
                // Top collision-vertex.
                float nX = c_qPos.x() + (c_nTessSz * TESS_X_SHIFT);
                float nY = c_qPos.y() - (c_nTessSz * TESS_Y_SHIFT);
                if ((*pIter)->PointInComb(QPoint(nX, nY))) { pvNeighbors[iCombIdx] = (*pIter); ++iCombIdx; continue; }

                // Top-right collision-vertex.
                nX += (c_nTessSz * HEX_LONG_SHORT) * HEX_HALF_WIDTH;
                nY += (c_nTessSz * HEX_SHORT_START);
                if ((*pIter)->PointInComb(QPoint(nX, nY))) { pvNeighbors[iCombIdx] = (*pIter); ++iCombIdx; continue; }

                // Bottom-right collision-vertex.
                nY += (c_nTessSz * HEX_LONG_SHORT);
                if ((*pIter)->PointInComb(QPoint(nX, nY))) { pvNeighbors[iCombIdx] = (*pIter); ++iCombIdx; continue; }

                // Bottom collision-vertex.
                nX -= (c_nTessSz * HEX_LONG_SHORT) * HEX_HALF_WIDTH;
                nY += (c_nTessSz * HEX_SHORT_START);
                if ((*pIter)->PointInComb(QPoint(nX, nY))) { pvNeighbors[iCombIdx] = (*pIter); ++iCombIdx; continue; }

                // Bottom-left collision-vertex.
                nX -= (c_nTessSz * HEX_LONG_SHORT) * HEX_HALF_WIDTH;
                nY -= (c_nTessSz * HEX_SHORT_START);
                if ((*pIter)->PointInComb(QPoint(nX, nY))) { pvNeighbors[iCombIdx] = (*pIter); ++iCombIdx; continue; }

                // Top-left collision-vertex.
                nY -= (c_nTessSz * HEX_LONG_SHORT);
                if ((*pIter)->PointInComb(QPoint(nX, nY))) { pvNeighbors[iCombIdx] = (*pIter); ++iCombIdx; continue; }
            }
        }
    }

    return pvNeighbors;
}

/*!
 * \brief CBoard::GetNeighbors [overloaded]
 *
 * This function will return all the neighbors for the given honeycomb index.
 *
 * \param uCombIdx - The index of the comb to gather the neighbors of.
 * \return CHoneyComb pointer array of the neighbors, or nullptr if no neighbors.
 */
CHoneyComb** CBoard::GetNeighbors(u32 uCombIdx)
{
    return GetNeighbors(GetComb(uCombIdx));
}

/*!
 * \brief CBoard::SetBoardSize
 *
 * This method is used to set the number of tessellation layers the board should use. These layers are the number of "cirlces" that go around the center honeycomb.
 * Default is 2 for a "medium" board. A large board would be "4" and a small board would be "1".
 *
 * \param[in] uSz - Number of layers.
 */
void CBoard::SetBoardSize(u32 uSz)
{
    miSize = uSz;
}

/*!
 * \brief CBoard::CalcTessPos
 *
 * This method is used to calculate all the positions of the honeycombs being tessellated for a given layer. These are calculated using a basic hexagon multiplied by the layer index.
 * Algorithm is as follows:
 *
 *  new QPointF[6 * (iLayerIdx+1)]
 *  for point in array:
 *      if point is hexagon vertex:
 *          calculate next vertex.
 *      calculate X shift amount divided by (layer + 1)
 *      calculate Y shift amount divided by (layer + 1)
 *      shift X by x-shift
 *      shift Y by y-shift
 *      Set point position to (x,y)
 *  done
 *
 * \param aStart - The start position (should be top-most comb).
 * \param iLayerIdx - The layer index we're at.
 * \return Pointer which references a heap-allocated array of QPointF references or nullptr upon error.
 */
std::vector<QPointF> CBoard::CalcTessPos(QPointF& aStart, u32 iLayerIdx, u32 uCellSz)
{
    // Simple variable to help us track half-height of the polygon.
    const float c_nFullCellSz = uCellSz * 2.0f;
    const float c_nCircumRadius = static_cast<float>(c_nFullCellSz * CELL_COMB_RATIO);
    const float c_nDegreePerAngle = 180.0f / 3.0f; // Should be 60.0f
    const u32 c_iNumPoints = NUM_HEX_VERTS * (iLayerIdx + 1);

    float nX = aStart.x();
    float nY = aStart.y();
    float nTheta = static_cast<float>(MAX_DEGREE - c_nDegreePerAngle); // We start with a negative degree.

    // Create the array.
    std::vector<QPointF> mpPointArr;

    // Calculate the next position.
    float nNxtVertRad = static_cast<float>(nTheta * (M_PI / 180.0f));
    float nNxtVertX = c_nCircumRadius * cos(nNxtVertRad);
    float nNxtVertY = c_nCircumRadius * sin(nNxtVertRad);

    // Begin calculating the points (clockwise, starting at top).
    //!\NOTE: Our hexagons have the long-leg vertical, meaning they're pointed at the top. (height > width)
    for (size_t iIdx = 0; c_iNumPoints > iIdx; ++iIdx)
    {
        // Set the comb position.
        mpPointArr.push_back(QPointF(nX, nY));

        // Calculate the next position.
        float nThetaRad = static_cast<float>(nTheta * (M_PI / 180.0f));
        float nXDelta = (c_nCircumRadius * cos(nThetaRad)) / (iLayerIdx + 1);
        float nYDelta = (c_nCircumRadius * sin(nThetaRad)) / (iLayerIdx + 1);

        // Add both.
        nX += nXDelta;
        nY += nYDelta;

        if (nXDelta == nNxtVertX && nYDelta == nNxtVertY)
        {
            nTheta += c_nDegreePerAngle;
            if (nTheta >= MAX_DEGREE)
            {
                nTheta -= MAX_DEGREE;
            }

            // Recalculate the next vertex.
            nNxtVertRad = static_cast<float>(nTheta * (M_PI / 180.0f));
            nNxtVertX = c_nCircumRadius * cos(nNxtVertRad);
            nNxtVertY = c_nCircumRadius * sin(nNxtVertRad);
        }
    }

    return mpPointArr;
}

import { useState, useCallback, useRef } from 'react';
import { Chessboard, defaultPieces } from 'react-chessboard';
import { Chess } from 'chess.js';

const ChessGame = () => {
    // create a chess game using a ref to always have access to the latest game state within closures and maintain the game state across renders
    const chessGameRef = useRef(new Chess());
    const chessGame = chessGameRef.current;

    // track the current position of the chess game in state to trigger a re-render of the chessboard
    const [chessPosition, setChessPosition] = useState(chessGame.fen());
    const [moveLog, setMoveLog] = useState([]);
    const [fenInput, setFenInput] = useState(chessGame.fen());

    const onDrop = useCallback((sourceSquare, targetSquare) => {
        try {
            const move = chessGame.move({
                from: sourceSquare,
                to: targetSquare,
                promotion: 'q'
            });

            if (move) {
                const moveNotation = `${game.turn() === 'w' ? 'Black' : 'White'}: ${move.san}`;
                setMoveLog(prev => [...prev, moveNotation]);
                return true;
            }
        } catch (error) {
            return false;
        }
        return false;
    }, [chessGame]);

    // make a random "CPU" move
    function makeRandomMove() {
        // get all possible moves`
        const possibleMoves = chessGame.moves();

        // exit if the game is over
        if (chessGame.isGameOver()) {
            return;
        }

        // pick a random move
        const randomMove = possibleMoves[Math.floor(Math.random() * possibleMoves.length)];

        // make the move
        const move = chessGame.move(randomMove);

        // update the position state
        setChessPosition(chessGame.fen());

        // update the fen input box
        setFenInput(chessGame.fen());

        // update the move log
        const moveNotation = `${chessGame.turn() === 'w' ? "Black" : "White"}: ${move.san}`;
        setMoveLog(prev => [...prev, moveNotation])
    }

    // handle piece drop
    function onPieceDrop({
        sourceSquare,
        targetSquare
    }) {
        // type narrow targetSquare potentially being null (e.g. if dropped off board)
        if (!targetSquare) {
            return false;
        }

        // try to make the move according to chess.js logic
        try {
            const move = chessGame.move({
                from: sourceSquare,
                to: targetSquare,
                promotion: 'q' // always promote to a queen for example simplicity
            });

            // update the position state upon successful move to trigger a re-render of the chessboard
            setChessPosition(chessGame.fen());

            const moveNotation = `${chessGame.turn() === 'w' ? 'Black' : 'White'}: ${move.san}`;
            setMoveLog(prev => [...prev, moveNotation]);

            // update the fen input box
            setFenInput(chessGame.fen());

            // make random cpu move after a short delay
            setTimeout(makeRandomMove, 500);

            // return true as the move was successful
            return true;
        } catch {
            // return false as the move was not successful
            return false;
        }
    }

    // set the chessboard options
    const chessboardOptions = {
        position: chessPosition,
        onPieceDrop,
        id: 'play-vs-random',
        lightSquareStyle: { backgroundColor: "#aaa" },
        darkSquareStyle: { backgroundColor: "#777" }
    };

    const moveLogStyle = {
        flex: 1,
        border: '1px solid #ccc',
        borderRadius: '4px',
        padding: '15px',
        minWidth: '200px',
        backgroundColor: '#dddddd',
        alignItem: "top",
    };

    const moveListStyle = {
        height: '400px',
        overflowY: 'auto',
        border: '1px solid #333',
        padding: '10px'
    };

    const moveItemStyle = {
        padding: '8px',
        borderBottom: '1px solid #333',
        backgroundColor: 'transparent'
    };

    const buttonStyle = {
        backgroundColor: "#1e9fe1",
        border: "none",
        borderRadius: "2px",
        color: "white",
        padding: "10px",
        textDecoration: "none",
        cursor: "pointer",
    }

    const inputStyle = {
        flex: "1",
        padding: "3px 10px",
        fontSize: "14px",
        borderRadius: "4px",
    }

    const changePosition = () => {
        try {
            chessGame.load(fenInput);
            setChessPosition(fenInput);
            if (fenInput != chessPosition)
                setMoveLog([]);
        } catch (e) {
            console.error("Invalid FEN string")
        }
        return;
    }

    // render the chessboard
    return (
        <div className='flex items-center space-x-6'>
            <div style={{ minWidth: '500px', mar: "10px", }}>
                <Chessboard options={chessboardOptions} />
                <div className='flex space-x-2 mt-2'>
                    <button style={buttonStyle} onClick={() => changePosition()}>Set Position</button>
                    <input name="fenInput" type="text" value={fenInput} style={inputStyle} onChange={(e) => setFenInput(e.target.value)} />
                </div>
            </div>
            <div style={moveLogStyle}>
                <h2 style={{ marginBottom: '15px', fontSize: '18px' }}>Move Log</h2>
                <div style={moveListStyle}>
                    {moveLog.length > 0 ? (
                        moveLog.map((move, index) => (
                            <div key={index} style={moveItemStyle}>
                                {`${Math.floor(index / 2) + 1}. ${move}`}
                            </div>
                        ))
                    ) : (
                        <div style={{ textAlign: 'center', color: '#666', fontStyle: 'italic' }}>
                            No moves yet
                        </div>
                    )}
                </div>
            </div>
        </div >

    );
};

export default ChessGame;
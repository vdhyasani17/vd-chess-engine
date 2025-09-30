import { useState } from 'react'
import reactLogo from './assets/react.svg'
import viteLogo from '/vite.svg'
import ChessGame from './ChessGame'

function App() {
  const [count, setCount] = useState(0)

  return (
    // Use min-h-screen instead of h-screen
    <div className="bg-[#19192a] w-full h-screen flex items-center justify-center">
      <div className="w-full flex justify-center" style={{ width: '500px' }}>
        <ChessGame />
      </div>
    </div>
  )
}
export default App